//
// Created by cory on 1/16/25.
//

#ifndef CLAMS_CONNECTION_HPP
#define CLAMS_CONNECTION_HPP


#include "writebuffer.hpp"
#include "entity/player.hpp"

#define CONN_BUFFER_SIZE 1024

class NetworkWorker;

enum State
{
    HANDSHAKE, STATUS, LOGIN, CONFIG, PLAY
};

/**
 * Handles connection state
 */
class Connection
{
public:
    const int fd;
    char rbuf[CONN_BUFFER_SIZE];

    explicit Connection(int client_fd);

    /**
     * Used to disconnect the Player. Internally, this method closes
     * the fd which also removes them from the epoll interest list.
     */
    ~Connection();

    inline void set_state(State connection_state) { state = connection_state; }

    [[nodiscard]] inline State get_state() const { return state; }

    void init_player(std::string username, UUID uuid);

    // STATUS

    void send_status_response();

    void send_pong_response();

    // LOGIN

    /**
     * id 0x00
     */
    void send_disconnect_login(const std::string& reason);

    /**
     * id 0x01
     */
    void send_encryption_request(const std::string& server_id, const std::string& key, const std::string& token, bool verify);

    /**
     * id 0x02
     */
    void send_login_success(const UUID& uuid, const std::string& username, const std::string& property);

    /**
     * id 0x03
     */
    void send_set_compression(int size);

    /**
     * id 0x04
     */
    void send_login_plugin_request();

    /**
     * id 0x05
     */
    void send_cookie_request_login();

    // CONFIG

    /**
     * id 0x00
     */
     void send_cookie_request_config();

     /**
      * id 0x01
      */
     void send_plugin_message_config();

     /**
      * id 0x02
      */
     void send_disconnect_config();

     void send_finish_config();

     void send_keep_alive_config();

     void send_ping_config();

     void send_reset_chat();

     void send_registry_data();

     void send_remove_resource_pack_config();

     void send_add_resource_pack_config();

     void send_store_cookie_config();

     void send_transfer();

     void send_feature_flags();

     void send_update_tags_config();

     void send_known_packs();

     void send_custom_report_details();

     void send_server_links_config();

     // PLAY

     void send_chunk_data_and_update_light(int cx, int cz/*, ChunkData& chunk, LightData& light*/);

     void set_health(float health, char food, float saturation);
private:
    State state;
    Player* handle;
    WriteBuffer wbuf;
};


#endif //CLAMS_CONNECTION_HPP
