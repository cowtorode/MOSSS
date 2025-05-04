//
// Created by cory on 4/11/25.
//

#ifndef CLAMS_READBUFFER_HPP
#define CLAMS_READBUFFER_HPP


#include <string>
#include "math/uuid.hpp"
#include "ivec.hpp"

class MalformedVarintException : std::exception {};

/**
 * Thrown when you try to read completely out of the bounds of the buffers fed to the ReadBuffer.
 */
class BufferOverflowException : std::exception {};

class ReadBuffer
{
public:
    ReadBuffer();

    [[nodiscard]] inline int sizeof_packet_length() const { return sizeof_length; }

    /**
     * @return The size (in bytes) of the length, packet identifier, and packet payload, computed as len + sizeof_len
     */
    [[nodiscard]] int total_packet_length() const;

    void feed(char* buffer);

    void make_secondary(long size);

    [[nodiscard]] char** primary_end();

    [[nodiscard]] char** secondary_end();

    char read_char();

    unsigned char read_uchar();

    inline bool read_bool() { return read_char(); }

    short read_short();

    unsigned short read_ushort();

    int read_int();

    unsigned long read_ulong();

    long read_long();

    float read_float();

    double read_double();

    int read_varint();

    int read_length();

    std::string read_string();

    UUID read_uuid();
private:
    void read_across_buffers(char* bytes, size_t remaining, size_t sizeof_type);

    void next_cursor();

    [[nodiscard]] inline size_t sector_remaining() const;

    /**
     * iv[0] is the primary buffer used for reading small packets and determining packet length.
     * If the total packet length exceeds the length of the primary buffer, a secondary buffer
     * (iv[1]) will be made to store the rest of the packet.
     */
    ivec iv[2];

    int cursor_index;
    ivec* cursor;

    /**
     * Packet length position
     */
    int pos;
    /**
     * Packet length
     */
    int packet_length;
    int sizeof_length;
};


#endif //CLAMS_READBUFFER_HPP
