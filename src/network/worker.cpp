//
// Created by cory on 1/16/25.
//

#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "network/tcpserver.hpp"
#include "network/worker.hpp"
#include "server.hpp"
#include "clamsutil.hpp"
#include "network/readbuffer.hpp"

#define MAX_EVENTS (128)

void handshake(Connection* conn, ReadBuffer& rbuf)
{
    int pvn = rbuf.read_varint();
    std::string ip = rbuf.read_string();
    unsigned short port = rbuf.read_ushort();
    int next_state = rbuf.read_varint();

    // TODO: DDoS protection?

    debug(std::cout << "handshake" << std::endl
                    << "  pvn:   " << pvn << std::endl
                    << "  ip:    " << ip << std::endl
                    << "  port:  " << port << std::endl
                    << "  state: " << next_state << std::endl;)

    switch (next_state)
    {
        case 1: // status
            conn->set_state(STATUS);
            break;
        case 2: // login
            conn->set_state(LOGIN);
            break;
        case 3: // transfer (NOT SUPPORTED)
        default:
            return;
    }
}

/**
 * status 0x00
 */
void status_request(Connection* conn, ReadBuffer& rbuf)
{
    debug(std::cout << "status_request" <<  std::endl;)
    conn->set_state(HANDSHAKE);
}

/**
 * status 0x01
 */
void ping_request(Connection* conn, ReadBuffer& rbuf)
{
    debug(std::cout << "ping_request" << std::endl;)
    conn->set_state(HANDSHAKE);
}

void login_start(Connection* conn, ReadBuffer& rbuf)
{
    std::string username = rbuf.read_string();
    UUID uuid = rbuf.read_uuid();

    debug(std::cout << "login_start" << std::endl
              << "  name: " << username << std::endl
              << "  uuid: " << uuid << std::endl;)

    // todo validate username and uuid with mojang api?

    conn->init_player(username, uuid);
    conn->send_set_compression(256);
    conn->send_login_success(uuid, username, "");
}

void encryption_response(Connection* conn, ReadBuffer& rbuf)
{
    std::string secret = rbuf.read_string();
    std::string token = rbuf.read_string();

    debug(std::cout << "encryption_response" << std::endl
                    << "  secret: " << secret << std::endl
                    << "  token: " << token << std::endl;)
}

void login_plugin_response(Connection* conn, ReadBuffer& rbuf)
{}

void login_acknowledged(Connection* conn, ReadBuffer& rbuf)
{
    debug(std::cout << "login_acknowledged" << std::endl;)

    conn->set_state(CONFIG);
}

void cookie_response(Connection* conn, ReadBuffer& rbuf)
{}

void client_information(Connection* conn, ReadBuffer& rbuf)
{
    std::string locale = rbuf.read_string();
    char view_distance = rbuf.read_char();
    char chat_mode = rbuf.read_char();
    bool chat_colors = rbuf.read_bool();
    unsigned char skin_parts = rbuf.read_char();
    char main_hand = rbuf.read_char();
    bool text_filtering = rbuf.read_bool();
    bool allow_server_listings = rbuf.read_bool();
    char particles = rbuf.read_char();
}

void cookie_response_config(Connection* conn, ReadBuffer& rbuf)
{}

void plugin_message(Connection* conn, ReadBuffer& rbuf)
{}

void acknowledge_finish_config(Connection* conn, ReadBuffer& rbuf)
{}

void keep_alive_config(Connection* conn, ReadBuffer& rbuf)
{}

void pong(Connection* conn, ReadBuffer& rbuf)
{}

void resource_pack_response(Connection* conn, ReadBuffer& rbuf)
{}

void known_packs(Connection* conn, ReadBuffer& rbuf)
{}

typedef void(*packet_parser)(Connection*, ReadBuffer&);

packet_parser handshaking[0x1] = {handshake};
packet_parser status[0x2] = {status_request, ping_request};
packet_parser login[0x5] = {
        login_start,
        encryption_response,
        login_plugin_response,
        login_acknowledged,
        cookie_response
};
packet_parser config[0x8] = {
        client_information,
        cookie_response_config,
        plugin_message,
        acknowledge_finish_config,
        keep_alive_config,
        pong,
        resource_pack_response,
        known_packs
};
packet_parser play[0x3a];

packet_parser* parsers[5] = {handshaking, status, login, config, play};

NetworkWorker::NetworkWorker() : epfd(epoll_create1(0))
{
    if (epfd == -1)
    {
        perror("NetworkWorker::NetworkWorker(): epoll_create1()");
    }
}

Connection* NetworkWorker::connect(int client_fd) const
{
    auto* conn = new Connection(client_fd);

    epoll_event event{};
    event.data.ptr = conn;
    event.events = EPOLLIN;

    // add the client to the epoll's interest list
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event) == -1)
    {
        perror("NetworkWorker::connect(): epoll_ctl(): EPOLL_CTL_ADD");
        disconnect(conn);
        return nullptr;
    }

    return conn;
}

NetworkWorker::~NetworkWorker()
{
    if (epfd != -1)
    {
        // kill the epoll_wait by writing to an interested fd with a null connection
        int efd = eventfd(0, EFD_NONBLOCK);
        if (efd == -1)
        {
            perror("NetworkWorker::~NetworkWorker(): eventfd()");
        } else
        {
            epoll_event event{};
            event.data.ptr = nullptr;
            event.events = EPOLLIN;
            if (epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &event) == -1)
            {
                perror("NetworkWorker::~NetworkWorker(): epoll_ctl(): EPOLL_CTL_ADD");
            } else
            {
                uint64_t u = 1;
                // send shutdown signal
                if (write(efd, &u, sizeof(u)) == -1)
                {
                    perror("NetworkWorker::~NetworkWorker(): write()");
                }
            }
        }

        // wait for it to finish
        runner.join();

        if (efd != -1)
        {
            close(efd);
        }

        close(epfd);
    }
}

void sigusr1(int no)
{
    logger().info("NetworkWorker::sigusr1(): received SIGUSR1");
}

void NetworkWorker::listen() const
{
    Connection* conn;
    ReadBuffer rbuf;
    epoll_event events[MAX_EVENTS];
    int nfds;
    ssize_t res;

    while (is_running())
    {
        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);

        if (nfds == -1)
        {
            perror("NetworkWorker::listen(): epoll_wait()");
            continue;
        }

        for (int i = 0; i < nfds; ++i)
        {
            // iterate through all of the epoll events waiting and process them
            conn = reinterpret_cast<Connection*>(events[i].data.ptr);

            if (!conn)
            {
                goto shutdown;
            }

            res = recv(conn->fd, conn->rbuf, CONN_BUFFER_SIZE, 0);

            if (res == -1)
            {
                perror("NetworkWorker::listen(): recv()");
                disconnect(conn);
                continue;
            }

            if (res == 0)
            {
                disconnect(conn);
                continue;
            }

            //debug(std::cout << res << " byte(s) for id " << packet_id << ": ";)

            // parsers[conn->get_state()][packet_id](conn, rbuf);
        }
    }
    shutdown:;
}

void NetworkWorker::start()
{
    runner = std::thread(&NetworkWorker::listen, this);
}
