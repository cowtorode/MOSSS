//
// Created by cory on 1/16/25.
//

#ifndef CLAMS_CONNECTION_HPP
#define CLAMS_CONNECTION_HPP


#define PRIMARY_BUFFER_SIZE (1024)

#include "entity/player.hpp"
#include "readbuffer.hpp"
#include "writebuffer.hpp"

enum State
{
    HANDSHAKE, STATUS, LOGIN, CONFIG, PLAY
};

/**
 * Handles connection state, reads, and writes
 */
class Connection
{
public:
    /**
     * Open client file descriptor that this Connection object manages reads, connection states, and writes for.
     */
    const int fd;
    char primary[PRIMARY_BUFFER_SIZE];
    /**
     * The address of the address of where data is being written for each recv() call. The reason this is a double
     * pointer is because this points to the buffer ends in the read buffer, and as it gets incremented those should
     * too.
     */
    char** write_cursor;
    /**
     * How many bytes are allowed to be copied from the socket receive buffer into userspace at write_cursor.
     */
    long bytes_allowed;
    /**
     * Handles reading the packet content using recv()
     *
     * 1. resolve_length(Connection*)
     * 2. copy_to_buffer(Connection*) if the packet can fit in the primary buffer and
     *                                it all wasn't read in the call to resolve_length
     *    (end)
     * 2. transition_to_secondary(Connection*) if the packet cannot all fit in the primary buffer.
     *                                         If that's the case, this method will handle the
     *                                         transition into the secondary buffer.
     * 3. copy_to_buffer(Connection*)
     *    (end)
     */
    void (*process_events)(Connection*);
    /**
     * Used for initial calls to process_events for parsing the packet length.
     */
    ReadBuffer rbuf;

    explicit Connection(int client_fd);

    /**
     * Used to disconnect the Player. Internally, this method closes
     * the fd which also removes them from the epoll interest list.
     */
    ~Connection();

    /**
     * Reset internal state to prepare for another read
     */
    void reset();

    /**
     * @return True if a packet is ready to be parsed, false if still waiting on data.
     */
    [[nodiscard]] bool ready() const;

    inline void set_state(State connection_state) { state = connection_state; }

    [[nodiscard]] inline State get_state() const { return state; }

    void init_player(std::string username, UUID uuid);

    [[nodiscard]] inline Player* player() const { return handle; }

    void write_wbuf();

    // STATUS
    void send_status_response(const std::string& response);

    void send_pong_response(long timestamp);

    // LOGIN
    void send_disconnect_login(const std::string& reason);

    void send_encryption_request(const std::string& server_id, const std::string& key, const std::string& token, bool verify);

    void send_login_success(const UUID& uuid, const std::string& username, const std::string& property);

    void send_set_compression(int size);

    void send_login_plugin_request();

    void send_cookie_request_login();

    // CONFIG

     void send_cookie_request_config(const std::string& identifer);

     // fixme
     void send_plugin_message_config(const std::string& channel, const std::string& data);

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

     void send_feature_flags(const std::string flags[], int len);

     void send_update_tags_config();

     void send_known_packs();

     void send_custom_report_details();

     void send_server_links_config();

     // PLAY

     void send_chunk_data_and_update_light(int cx, int cz/*, ChunkData& chunk, LightData& light*/);

     void send_set_health(float health, char food, float saturation);
private:
    State state;
    Player* handle;
    WriteBuffer wbuf;
};


#endif //CLAMS_CONNECTION_HPP
