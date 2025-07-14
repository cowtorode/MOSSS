//
// Created by cory on 1/16/25.
//

#define PRINT_SERVERBOUND

#ifdef PRINT_SERVERBOUND
#include <iostream>
#define debug(stmt) stmt
#elif
#define debug(stmt)
#endif

#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>
#include "network/tcpserver.hpp"
#include "network/worker.hpp"
#include "server.hpp"
#include "network/readbuffer.hpp"

// How many events an epoll_wait can handle at once.
// fixme bump this up?
#define MAX_EVENTS (128)
#define PROTOCOL_VERSION (770)

void handshake(Connection* conn, ReadBuffer& rbuf)
{
    int pvn = rbuf.read_varint();
    std::string ip = rbuf.read_string();
    unsigned short port = rbuf.read_ushort();
    int next_state = rbuf.read_varint();

    // TODO: DDoS protection?

    debug(
    logger().info("[%i > S] handshake", conn->fd);
    logger().info("  pvn:   %i", pvn);
    logger().info("  ip:    %s", ip.c_str());
    logger().info("  port:  %i", port);
    logger().info("  state: %i", next_state);)

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
    debug(logger().info("[%i > S] status_request", conn->fd);)

    //{
    //  "version":
    //  {
    //    "name":"1.21.4",
    //    "protocol":769
    //  },
    //  "enforcesSecureChat":true,
    //  "description":"A.Minecraft.Server",
    //  "players":
    //  {
    //    "max":20,
    //    "online":0
    //  }
    //}
    conn->send_status_response(R"({"version":{"name":"1.21.4","protocol":769},"enforcesSecureChat":true,"description":"A Minecraft Server","players":{"max":20,"online":0}})");
}

/**
 * status 0x01
 */
void ping_request(Connection* conn, ReadBuffer& rbuf)
{
    long time = rbuf.read_long();

    debug(
    logger().info("[%i > S] ping_request", conn->fd);
    logger().info("  time: %li", time);)

    conn->send_pong_response(time);
    conn->set_state(HANDSHAKE);
}

void login_start(Connection* conn, ReadBuffer& rbuf)
{
    std::string username = rbuf.read_string();
    UUID uuid = rbuf.read_uuid();

    debug(
    logger().info("[%i > S] login_start", conn->fd);
    logger().info("  name: %s", username.c_str());
    std::cout << "  uuid: " << uuid << std::endl;)

    // todo validate name and uuid with mojang api?

    conn->init_player(username, uuid);
    //conn->send_set_compression(256);
    // todo fixme uuid
    conn->send_login_success(conn->player()->uuid(), conn->player()->username(), "");
}

void encryption_response(Connection* conn, ReadBuffer& rbuf)
{
    std::string secret = rbuf.read_string();
    std::string token = rbuf.read_string();

    debug(
    logger().info("[%i > S] encryption_response", conn->fd);
    logger().info("  secret: %s", secret.c_str());
    logger().info("  token: %s", token.c_str());)
}

void login_plugin_response(Connection* conn, ReadBuffer& rbuf)
{
    debug(logger().info("[%i > S] login_plugin_response", conn->fd);)
}

void login_acknowledged(Connection* conn, ReadBuffer& rbuf)
{
    debug(logger().info("[%i > S] login_acknowledged", conn->fd);)

    conn->set_state(CONFIG);
}

void cookie_response(Connection* conn, ReadBuffer& rbuf)
{
    debug(logger().info("[%i > S] cookie_response", conn->fd);)
}

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

    debug(
    logger().info("[%i > S] client_information", conn->fd);
    logger().info("  view_distance: %i", view_distance);
    logger().info("  chat_mode: %i", chat_mode);
    logger().info("  chat_colors: %i", chat_colors);
    logger().info("  skin_parts: %i", skin_parts);
    logger().info("  main_hand: %i", main_hand);
    logger().info("  text_filtering: %i", text_filtering);
    logger().info("  allow_server_listings: %i", allow_server_listings);
    logger().info("  particles: %i", particles);)

    conn->send_plugin_message_config("minecraft:brand", "CLAMS");
    std::string flags[] = {"minecraft:vanilla"};
    conn->send_feature_flags(flags, 1);
    conn->send_finish_config();
}

void cookie_response_config(Connection* conn, ReadBuffer& rbuf)
{
    debug(logger().info("[%i > S] cookie_response_config", conn->fd);)
}

void brand(Connection* conn, ReadBuffer& rbuf)
{
    std::string brand = rbuf.read_string();

    debug(logger().info("  brand: %s", brand.c_str());)
}

typedef void(*channel_handler)(Connection*, ReadBuffer&);
std::unordered_map<std::string, channel_handler> channel_handlers = {{"minecraft:brand", brand}};

void plugin_message(Connection* conn, ReadBuffer& rbuf)
{
    std::string channel = rbuf.read_string();
    auto itr = channel_handlers.find(channel);

    debug(
    logger().info("[%i > S] plugin_message", conn->fd);
    logger().info("  channel: %s", channel.c_str());)

    if (itr == channel_handlers.end())
    {
        logger().err("Unknown plugin channel: %s", channel.c_str());
    } else
    {
        itr->second(conn, rbuf);
    }
}

void acknowledge_finish_config(Connection* conn, ReadBuffer& rbuf)
{
    debug(logger().info("[%i > S] acknowledge_finish_config", conn->fd);)

    conn->set_state(PLAY);
}

void keep_alive_config(Connection* conn, ReadBuffer& rbuf)
{
    debug(logger().info("[%i > S] keep_alive_config", conn->fd);)
}

void pong(Connection* conn, ReadBuffer& rbuf)
{
    debug(logger().info("[%i > S] pong_config", conn->fd);)
}

void resource_pack_response(Connection* conn, ReadBuffer& rbuf)
{
    debug(logger().info("[%i > S] resource_pack_response", conn->fd);)
}

void known_packs(Connection* conn, ReadBuffer& rbuf)
{
    debug(logger().info("[%i > S] known_packs", conn->fd);)
}

void play_confirm_teleportation(Connection* conn, ReadBuffer& rbuf) {}
void play_query_block_entity_tag(Connection* conn, ReadBuffer& rbuf) {}
void play_bundle_item_selected(Connection* conn, ReadBuffer& rbuf) {}
void play_change_difficulty(Connection* conn, ReadBuffer& rbuf) {}
void play_acknowledge_message(Connection* conn, ReadBuffer& rbuf) {}
void play_chat_command(Connection* conn, ReadBuffer& rbuf) {}
void play_signed_chat_command(Connection* conn, ReadBuffer& rbuf) {}
void play_chat_message(Connection* conn, ReadBuffer& rbuf) {}
void play_player_session(Connection* conn, ReadBuffer& rbuf) {}
void play_chunk_batch_received(Connection* conn, ReadBuffer& rbuf) {}
void play_client_status(Connection* conn, ReadBuffer& rbuf) {}

void play_client_tick_end(Connection* conn, ReadBuffer& rbuf)
{
    // do nothing
}

void play_client_information(Connection* conn, ReadBuffer& rbuf) {}
void play_command_suggestions_request(Connection* conn, ReadBuffer& rbuf) {}
void play_acknowledge_configuration(Connection* conn, ReadBuffer& rbuf) {}
void play_click_container_button(Connection* conn, ReadBuffer& rbuf) {}
void play_click_container(Connection* conn, ReadBuffer& rbuf) {}
void play_close_container(Connection* conn, ReadBuffer& rbuf) {}
void play_change_container_slot_state(Connection* conn, ReadBuffer& rbuf) {}
void play_cookie_response(Connection* conn, ReadBuffer& rbuf) {}
void play_plugin_message(Connection* conn, ReadBuffer& rbuf) {}
void play_debug_sample_subscription(Connection* conn, ReadBuffer& rbuf) {}
void play_edit_book(Connection* conn, ReadBuffer& rbuf) {}
void play_query_entity_tag(Connection* conn, ReadBuffer& rbuf) {}
void play_interact(Connection* conn, ReadBuffer& rbuf) {}
void play_jigsaw_generate(Connection* conn, ReadBuffer& rbuf) {}
void play_keep_alive(Connection* conn, ReadBuffer& rbuf) {}
void play_lock_difficulty(Connection* conn, ReadBuffer& rbuf) {}
void play_set_player_position(Connection* conn, ReadBuffer& rbuf) {}
void play_set_player_position_rotation(Connection* conn, ReadBuffer& rbuf) {}
void play_set_player_rotation(Connection* conn, ReadBuffer& rbuf) {}
void play_set_player_movement_flags(Connection* conn, ReadBuffer& rbuf) {}
void play_move_vehicle(Connection* conn, ReadBuffer& rbuf) {}
void play_paddle_boat(Connection* conn, ReadBuffer& rbuf) {}
void play_pick_item_from_block(Connection* conn, ReadBuffer& rbuf) {}
void play_pick_item_from_entity(Connection* conn, ReadBuffer& rbuf) {}
void play_ping_request(Connection* conn, ReadBuffer& rbuf) {}
void play_place_recipe(Connection* conn, ReadBuffer& rbuf) {}
void play_player_abilities(Connection* conn, ReadBuffer& rbuf) {}
void play_player_action(Connection* conn, ReadBuffer& rbuf) {}
void play_player_command(Connection* conn, ReadBuffer& rbuf) {}
void play_player_input(Connection* conn, ReadBuffer& rbuf) {}
void play_player_loaded(Connection* conn, ReadBuffer& rbuf) {}
void play_pong(Connection* conn, ReadBuffer& rbuf) {}
void play_change_recipe_book_settings(Connection* conn, ReadBuffer& rbuf) {}
void play_set_seen_recipe(Connection* conn, ReadBuffer& rbuf) {}
void play_rename_item(Connection* conn, ReadBuffer& rbuf) {}
void play_resource_pack_response(Connection* conn, ReadBuffer& rbuf) {}
void play_seen_advancements(Connection* conn, ReadBuffer& rbuf) {}
void play_select_trade(Connection* conn, ReadBuffer& rbuf) {}
void play_set_beacon_effect(Connection* conn, ReadBuffer& rbuf) {}
void play_set_held_item(Connection* conn, ReadBuffer& rbuf) {}
void play_program_command_block(Connection* conn, ReadBuffer& rbuf) {}
void play_program_command_block_minecart(Connection* conn, ReadBuffer& rbuf) {}
void play_set_creative_mode_slot(Connection* conn, ReadBuffer& rbuf) {}
void play_program_jigsaw_block(Connection* conn, ReadBuffer& rbuf) {}
void play_program_structure_block(Connection* conn, ReadBuffer& rbuf) {}
void play_set_test_block(Connection* conn, ReadBuffer& rbuf) {}
void play_update_sign(Connection* conn, ReadBuffer& rbuf) {}
void play_swing_arm(Connection* conn, ReadBuffer& rbuf) {}
void play_teleport_to_entity(Connection* conn, ReadBuffer& rbuf) {}
void play_test_instance_block_action(Connection* conn, ReadBuffer& rbuf) {}
void play_use_item_on(Connection* conn, ReadBuffer& rbuf) {}
void play_use_item(Connection* conn, ReadBuffer& rbuf) {}

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
packet_parser play[0x40] = {
        play_confirm_teleportation,
        play_query_block_entity_tag,
        play_bundle_item_selected,
        play_change_difficulty,
        play_acknowledge_message,
        play_chat_command,
        play_signed_chat_command,
        play_chat_message,
        play_player_session,
        play_chunk_batch_received,
        play_client_status,
        play_client_tick_end,
        play_client_information,
        play_command_suggestions_request,
        play_acknowledge_configuration,
        play_click_container_button,
        play_click_container,
        play_close_container,
        play_change_container_slot_state,
        play_cookie_response,
        play_plugin_message,
        play_debug_sample_subscription,
        play_edit_book,
        play_query_entity_tag,
        play_interact,
        play_jigsaw_generate,
        play_keep_alive,
        play_lock_difficulty,
        play_set_player_position,
        play_set_player_position_rotation,
        play_set_player_rotation,
        play_set_player_movement_flags,
        play_move_vehicle,
        play_paddle_boat,
        play_pick_item_from_block,
        play_pick_item_from_entity,
        play_ping_request,
        play_place_recipe,
        play_player_abilities,
        play_player_action,
        play_player_command,
        play_player_input,
        play_player_loaded,
        play_pong,
        play_change_recipe_book_settings,
        play_set_seen_recipe,
        play_rename_item,
        play_resource_pack_response,
        play_seen_advancements,
        play_select_trade,
        play_set_beacon_effect,
        play_set_held_item,
        play_program_command_block,
        play_program_command_block_minecart,
        play_set_creative_mode_slot,
        play_program_jigsaw_block,
        play_program_structure_block,
        play_set_test_block,
        play_update_sign,
        play_swing_arm,
        play_teleport_to_entity,
        play_test_instance_block_action,
        play_use_item_on,
        play_use_item
};

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
        perror("NetworkWorker::connect(int): epoll_ctl(): EPOLL_CTL_ADD");
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

void NetworkWorker::listen() const
{
    Connection* conn;
    epoll_event events[MAX_EVENTS];
    int nfds;

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
            // iterate through all the epoll events waiting to process them
            conn = reinterpret_cast<Connection*>(events[i].data.ptr);

            if (!conn)
            {
                goto shutdown;
            }

            try
            {
                conn->process_events(conn);

                // Note: If the connection is disconnected, conn->ready() will be called, but it will
                // always fail

                /* If the connection is ready for parsing, meaning
                   that it has copied all the data from the OS, */
                if (conn->ready())
                {
                    // Length is already read (that's a part of conn::process_events())
                    char packet_id = conn->rbuf.read_char();

                    // todo packet compression

                    parsers[conn->get_state()][packet_id](conn, conn->rbuf);

                    conn->reset();
                }
            } catch (const MalformedVarintException& mve)
            {
                logger().err("MalformedVarintException");
                disconnect(conn);
            } catch (const BufferOverflowException& boe)
            {
                logger().err("BufferOverflowException");
                disconnect(conn);
            }
        }
    }
    shutdown:;
}

void NetworkWorker::start()
{
    runner = std::thread(&NetworkWorker::listen, this);
}
