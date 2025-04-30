//
// Created by cory on 1/16/25.
//

#include <unistd.h>
#include <sys/uio.h>
#include "network/connection.hpp"
#include "network/clientbound_packet_ids.h"
#include "clamsutil.hpp"

Connection::Connection(int client_fd) : fd(client_fd),
                                        state(HANDSHAKE),
                                        handle(nullptr),
                                        wbuf(16)
{
    // This doesn't need to initialize the rbuf
}

Connection::~Connection()
{
    delete handle;
    // this also removes them from the epoll interest list
    close(fd);
}

void Connection::init_player(std::string username, UUID uuid)
{
    handle = new Player(uuid, username);
}

void Connection::send_disconnect_login(const std::string& reason)
{
    wbuf.write_byte(LOGIN_DISCONNECT); // id
    wbuf.write_string(reason);

    writev(fd, wbuf.finalize(), wbuf.iov_size());

    wbuf.reset();
}

void Connection::send_encryption_request(const std::string& server_id, const std::string& key, const std::string& token, bool verify)
{
    wbuf.write_byte(LOGIN_ENCRYPTION_REQUEST); // id
    wbuf.write_string(server_id);
    wbuf.write_string(key);
    wbuf.write_string(token);
    wbuf.write_bool(verify);

    writev(fd, wbuf.finalize(), wbuf.iov_size());

    wbuf.reset();
}

void Connection::send_login_success(const UUID& uuid, const std::string& username, const std::string& property)
{
    unsigned char packet[26] = {0x00, 0x02, 0xb0, 0x74, 0x6b, 0x73, 0xd5, 0x61, 0x31, 0x9a, 0xb5, 0x50, 0x6c, 0x27, 0x3b, 0x88, 0xd7, 0x77, 0x06, 0x50, 0x76, 0x62, 0x62, 0x6c, 0x65, 0x00};
    wbuf.write_bytes(reinterpret_cast<char*>(packet), 26);
//    wbuf.write_byte(0x02); // id
//    wbuf.write_uuid(uuid);
//    wbuf.write_string(username);
//    wbuf.write_string(property);

    writev(fd, wbuf.finalize(), wbuf.iov_size());

    wbuf.reset();
}

void Connection::send_set_compression(int size)
{
    wbuf.write_byte(LOGIN_SET_COMPRESSION);
    wbuf.write_varint(size);

    writev(fd, wbuf.finalize(), wbuf.iov_size());

    wbuf.reset();
}
