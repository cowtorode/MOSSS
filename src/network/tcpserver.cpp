//
// Created by cory on 1/15/25.
//

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include "network/tcpserver.hpp"
#include "server.hpp"
#include "clamsutil.hpp"
#include "network/worker.hpp"
#include <vector>

std::string make_public_key()
{
    // Generate RSA key
    RSA* rsa = RSA_new();
    BIGNUM* bn = BN_new();
    BN_set_word(bn, RSA_F4); // RSA_F4 = 65537

    if (!RSA_generate_key_ex(rsa, 1024, bn, nullptr))
    {
        logger().err("Public encryption key generation failed.");
        return "";
    }

    // Convert public key to DER format
    unsigned char* der_buf = nullptr;
    int der_len = i2d_RSA_PUBKEY(rsa, &der_buf); // Note: advances the pointer

    if (der_len <= 0)
    {
        logger().err("ASN.1 DER public encryption key conversion failed.");
        return "";
    }

    std::string result(reinterpret_cast<char*>(der_buf), der_len);

    // Cleanup
    RSA_free(rsa);
    BN_free(bn);
    OPENSSL_free(der_buf);

    return result;
}

std::string public_key;

std::thread runner;

std::size_t worker_length;
NetworkWorker* workers;

int server_fd = -1;
// todo make threadsafe
std::vector<Connection*> connections;

const std::string& get_public_key()
{
    return public_key;
}

debug(
static void print_disconnect(Connection* conn)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    if (getpeername(conn->fd, (sockaddr*) &addr, &len) == -1)
    {
        logger().info("[ - ]: (%i)", conn->fd);
    } else
    {
        logger().info("[ - ]: /%s:%uh (%i)", inet_ntoa(addr.sin_addr), htons(addr.sin_port), conn->fd);
    }
})

void disconnect(Connection* conn)
{
    debug(print_disconnect(conn);)

    connections.erase(std::find(connections.begin(), connections.end(), conn));
    delete conn;
}

void disconnect_all()
{
    for (auto* conn : connections)
    {
        debug(print_disconnect(conn);)
        delete conn;
    }
    connections.clear();
}

bool tcp_init(unsigned short port)
{
    /* Setup our network environment, */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("tcp_init(unsigned short): socket()");
        return false;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("tcp_init(unsigned short): setsockopt(): SO_REUSEADDR");
        return false;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*) &server_addr, sizeof(server_addr)) == -1)
    {
        perror("tcp_init(unsigned short): bind()");
        return false;
    }

    /* await incoming client connections */
    if (listen(server_fd, 128) == -1)
    {
        perror("tcp_init(unsigned short): listen()");
        return false;
    }

    public_key = make_public_key();
    debug(logger().info("Generated public encryption key (%i bytes)", public_key.size());)

    worker_length = 1;
    workers = new NetworkWorker[worker_length];
    return true; /* Network is set up and ready to run. */
}

static NetworkWorker& next_worker()
{
    return workers[0];
}

void start_workers()
{
    for (int i = 0; i < worker_length; ++i)
    {
        workers[i].start();
    }
}

static inline void configure_fd(int client_fd)
{
    int opt = 1;
    int flags = fcntl(client_fd, F_GETFL, 0);

    if (flags == -1)
    {
        perror("configure_fd(int): fcntl(): get");
    }

    // set the client to nonblocking for the worker
    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("configure_fd(int): fcntl(): set");
    }

    if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)))
    {
        perror("configure_fd(int): setsockopt(): TCP_NODELAY");
    }
}

void accept_connections()
{
    int client_fd;

    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (is_running())
    {
        // This blocks until a client attempts to connect
        client_fd = accept(server_fd, (sockaddr*) &client_addr, &client_len);

        if (client_fd == -1)
        {
            // If the server is shutting down, we don't have to error.
            if (is_running())
            {
                perror("accept_connections(): accept()");
            }
            continue;
        }

        configure_fd(client_fd);

        Connection* conn = next_worker().connect(client_fd);

        if (conn)
        {
            connections.push_back(conn);
            debug(logger().info("[ + ]: /%s:%uh (%i)", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port), client_fd);)
        } else
        {
            close(client_fd);
            logger().err("Failed to ready client_fd %i", client_fd);
        }
    }
}

void tcp_start()
{
    start_workers();

    runner = std::thread(accept_connections);
}

void tcp_stop()
{
    if (server_fd != -1)
    {
        // stop new connections from coming in
        shutdown(server_fd, SHUT_RDWR);
        // disconnect all current clients
        disconnect_all();
        close(server_fd);
    }

    delete[] workers;

    runner.join();
}
