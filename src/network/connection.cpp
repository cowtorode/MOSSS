//
// Created by cory on 1/16/25.
//

#include <unistd.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include "network/connection.hpp"
#include "network/clientbound_packet_ids.hpp"
#include "clamsutil.hpp"
#include "network/tcpserver.hpp"
#include "logger.hpp"

// Primary buffer
// 256 B           256 B           256 B           256 B
// |              ||              ||              ||              |

void copy_to_buffer(Connection* conn)
{
    ssize_t res = recv(conn->fd, *conn->write_cursor, conn->bytes_allowed, 0);

    if (res <= 0)
    {
        if (res == -1)
        {
            perror("copy_to_buffer(Connection*): recv()");
        }
        disconnect(conn);
    } else
    {
        *conn->write_cursor += res;
        conn->bytes_allowed -= res;
    }
}

void transition_to_secondary(Connection* conn)
{
    ssize_t res = recv(conn->fd, *conn->write_cursor, conn->bytes_allowed, 0);

    if (res <= 0)
    {
        if (res == -1)
        {
            perror("transition_to_secondary(Connection*): recv()");
        }
        disconnect(conn);
    } else
    {
        *conn->write_cursor += res;

        // This should never be less than res, but I hate my life, so I wrote it anyway
        // if bytes_allowed is equal to (or less than) res, it means that we just filled
        // the primary buffer all the way, and we need to transition into the secondary
        // buffer to finish copying the packet from kernel space to user space.
        if (conn->bytes_allowed <= res)
        {
            // size of the secondary buffer
            conn->bytes_allowed = conn->rbuf.total_packet_length() - PRIMARY_BUFFER_SIZE;

            // todo alternatively we can get the buffer from a buffer pool which would technically be faster
            conn->rbuf.make_secondary(conn->bytes_allowed);;
            conn->write_cursor = conn->rbuf.secondary_end();

            // read into the buffer
            conn->process_events = copy_to_buffer;
        }
    }
}

void resolve_length(Connection* conn)
{
    // Copy res number of bytes from the OS
    // bytes_allowed, while can be zero or negative, will never be at this line. That's because
    // after each return of epoll_wait() and call to conn->process_events(conn), if bytes_allowed
    // is less than or equal to zero, the packet read is parsed and bytes_allowed gets reset to
    // PRIMARY_BUFFER_SIZE
    ssize_t res = recv(conn->fd, *conn->write_cursor, conn->bytes_allowed, 0);

    if (res <= 0)
    {
        if (res == -1)
        {
            perror("resolve_length(Connection*): recv()");
        }
        disconnect(conn);
    } else
    {
        /* We need to shift where we're writing next to account
           for the number of bytes that we just read. */
        *conn->write_cursor += res;
        /* and we also need to account for the number of bytes
           that are no longer available to be written to. */
        conn->bytes_allowed -= res;

        /*
           Since 'bytes_allowed' permits the full primary buffer to be filled, it's possible
           for recv() to read not just the current packet, but also bytes from subsequent packets.
           We can't parse those extra bytes yet, but we must preserve them.

           This is resolved once the packet length is known (via resolve_length), at which point
           recv() can be limited to only the expected number of bytes.

           Until then, for small packets, any extra data read must be copied back to the start of
           the primary buffer after parsing, so parsing can continue correctly with the next packet.
        */

        int len = conn->rbuf.read_length();

        if (len != -1)
        {
            // We have the length of the packet
            if (PRIMARY_BUFFER_SIZE - conn->rbuf.sizeof_packet_length() < len)
            {
                // The packet is going to require a secondary buffer :(
                conn->process_events = transition_to_secondary;
            } else
            {
                /*
                   bytes_to_read =
                   Number of bytes in total expected to read from the TCP stream from just
                   this one packet (including len, id, and payload), which computes to be
                   len + sizeof_len, minus the number of bytes already read. This will give
                   us the number of bytes that still need read.

                   If this quantity is negative, we read too many bytes, as discussed in the
                   large comment above. This will be communicated to ready() to parse the
                   data in the packet, and it will communicate to reset() how many bytes need
                   copied to the beginning of the primary.
                */
                conn->bytes_allowed = conn->rbuf.total_packet_length() - (*conn->write_cursor - conn->primary);
                conn->process_events = copy_to_buffer;

                // if (bytes_to_read < 0) we read too much :(
                // ready() will return true since bytes_allowed is <= 0
            }
        }
    }
}

Connection::Connection(int client_fd) : fd(client_fd),
                                        bytes_allowed(PRIMARY_BUFFER_SIZE),
                                        process_events(resolve_length),
                                        rbuf(),
                                        state(HANDSHAKE),
                                        handle(nullptr),
                                        wbuf(16)
{
    // This doesn't need to initialize primary
    rbuf.feed(primary);
    write_cursor = rbuf.primary_end();
}

Connection::~Connection()
{
    delete handle;
    // this also removes them from the epoll interest list
    close(fd);
}

void Connection::reset()
{
    long overflow = -bytes_allowed;

    // copy any overflow to the beginning of primary
    if (overflow > 0)
    {
        int previous_packet_length = rbuf.total_packet_length();
        for (int i = 0; i < -bytes_allowed; ++i)
        {
            primary[i] = primary[previous_packet_length + i];
        }
        rbuf.feed(primary + overflow);
    } else
    {
        rbuf.feed(primary);
    }

    write_cursor = rbuf.primary_end();
    bytes_allowed = PRIMARY_BUFFER_SIZE;
    process_events = resolve_length;
}

bool Connection::ready() const
{
    return bytes_allowed <= 0;
}

void Connection::init_player(std::string username, UUID uuid)
{
    handle = new Player(uuid, username);
}

void Connection::write_wbuf()
{
    iovec* finalized = wbuf.iov();
    iovec* iov = finalized;
    int iov_size = wbuf.iov_size();
    int iov_cursor = 0;

    while (iov_cursor < iov_size)
    {
        ssize_t res = writev(fd, iov, iov_size - iov_cursor);

        if (res == -1)
        {
            perror("Connection::write_wbuf(): writev()");
            break;
        }

        // While the number of bytes written is greater than or equal to the bytes in the iovec,
        while (res >= iov->iov_len)
        {
            res -= static_cast<ssize_t>(iov->iov_len);
            // this iovec is getting skipped so we don't need to zero the iov_len
            // iov->iov_len = 0;
            ++iov_cursor;
            iov = finalized + iov_cursor;
        }

        iov->iov_len -= res;
        iov->iov_base = reinterpret_cast<char*>(iov->iov_base) + res;
    }

    wbuf.reset();
}

void Connection::send_status_response(const std::string& response)
{
    wbuf.write_byte(STATUS_RESPONSE);
    wbuf.write_string(response);

    debug(logger().info("[S > %i] status_response", fd);)
    write_wbuf();
}

void Connection::send_pong_response(long timestamp)
{
    wbuf.write_byte(STATUS_PONG_RESPONSE);
    wbuf.write_long(timestamp);

    debug(logger().info("[S > %i] pong_response", fd);)
    write_wbuf();
}

void Connection::send_disconnect_login(const std::string& reason)
{
    wbuf.write_byte(LOGIN_DISCONNECT);
    wbuf.write_string(reason);

    write_wbuf();
}

void Connection::send_encryption_request(const std::string& server_id, const std::string& key, const std::string& token, bool verify)
{
    wbuf.write_byte(LOGIN_ENCRYPTION_REQUEST);
    wbuf.write_string(server_id);
    wbuf.write_string(key);
    wbuf.write_string(token);
    wbuf.write_bool(verify);

    write_wbuf();
}

void Connection::send_login_success(const UUID& uuid, const std::string& username, const std::string& property)
{
    wbuf.write_byte(LOGIN_SUCCESS);
    wbuf.write_uuid(uuid);
    wbuf.write_string(username);
    wbuf.write_string(property);

    write_wbuf();
}

void Connection::send_set_compression(int size)
{
    wbuf.write_byte(LOGIN_SET_COMPRESSION);
    wbuf.write_varint(size);

    write_wbuf();
}

void Connection::send_cookie_request_config(const std::string& identifer)
{

}

void Connection::send_plugin_message_config(const std::string& channel, const std::string& data)
{
    wbuf.write_byte(CONFIG_PLUGIN_MESSAGE);
    wbuf.write_string(channel);
    wbuf.write_string(data);

    write_wbuf();
}

void Connection::send_finish_config()
{
    wbuf.write_byte(CONFIG_FINISH);

    write_wbuf();
}

void Connection::send_feature_flags(const std::string* flags, int len)
{
    wbuf.write_byte(CONFIG_FEATURE_FLAGS);
    wbuf.write_varint(len);

    for (int i = 0; i < len; ++i)
    {
        wbuf.write_string(flags[i]);
    }

    write_wbuf();
}
