//
// Created by cory on 4/11/25.
//

#include <bit>
#include "network/readbuffer.hpp"

#define SEGMENT_BITS (0b01111111)
#define CONTINUE_BIT (0b10000000)

ReadBuffer::ReadBuffer() : iv(), cursor_index(0), cursor(iv), pos(0), packet_length(0), sizeof_length(0)
{}

inline size_t ReadBuffer::sector_remaining() const
{
    return cursor->end - cursor->buf;
}

int ReadBuffer::total_packet_length() const
{
    return packet_length + sizeof_length;
}

void ReadBuffer::feed(char* buffer)
{
    iv->buf = buffer;
    iv->end = buffer;
    cursor_index = 0;
    cursor = iv;
    pos = 0;
    packet_length = 0;
    sizeof_length = 0;
    delete[] iv[1].buf;
}

void ReadBuffer::make_secondary(long size)
{
    iv[1].buf = new char[size];
    iv[1].end = iv[1].buf;
}

void ReadBuffer::next_cursor()
{
    ++cursor_index;
    if (cursor_index > 2)
    {
        throw BufferOverflowException();
    }
    cursor = iv + cursor_index;
}

char ReadBuffer::read_char()
{
    if (sector_remaining() < sizeof(char))
    {
        next_cursor();
    }

    return *cursor->buf++;
}

int ReadBuffer::read_varint()
{
    int value = 0;
    int position = 0;
    char c;

    while (true)
    {
        c = read_char();
        // pos = 0     pos = 7     pos = 14    pos = 21    pos = 28   pos = 35 (ERR)
        // 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b00000001
        // for the last byte, the most significant half byte has to be 0000.
        value |= (c & SEGMENT_BITS) << position;

        if (!(c & CONTINUE_BIT))
        {
            break;
        }

        position += 7;

        if (position > 21)
        {
            // we're reading the very last char, so we need to check for malformed data
            c = read_char();
            if ((c & 0xf0))
            {
                /* The varint is trying to compile more than 5 bytes or data
                   in the last byte which is too large to fit into an int */
                throw MalformedVarintException();
            }

            value |= (c & SEGMENT_BITS) << position;
            break;
        }
    }

    return value;
}

int ReadBuffer::read_length()
{
    const char* end = iv->end;
    char c;

    if (pos > 21)
    {
        goto last_byte;
    }

    while (true)
    {
        if (iv->buf >= end)
        {
            return -1;
        }
        c = *iv->buf++;
        ++sizeof_length;

        // pos = 0     pos = 7     pos = 14    pos = 21    pos = 28   pos = 35 (ERR)
        // 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b00000001
        // for the last byte, the most significant half byte has to be 0000.
        packet_length |= (c & SEGMENT_BITS) << pos;

        if (!(c & CONTINUE_BIT))
        {
            break;
        }

        pos += 7;

        if (pos > 21)
        {
            last_byte:
            if (iv->buf >= end)
            {
                return -1;
            }
            c = *iv->buf++;
            ++sizeof_length;

            // we're reading the very last char, so we need to check for malformed data
            if ((c & 0xf0))
            {
                /* The varint is trying to compile more than 5 bytes or data
                   in the last byte which is too large to fit into an int */
                throw MalformedVarintException();
            }

            packet_length |= (c & SEGMENT_BITS) << pos;
            break;
        }
    }

    return packet_length;
}

void ReadBuffer::read_across_buffers(char* bytes, size_t remaining, size_t sizeof_type)
{
    // how far into the secondary we're going
    size_t into_secondary = sizeof_type - remaining;

    // in the case that we're midway between buffers, you need to copy byte by byte in the right order
    for (int i = 0; i < remaining; ++i)
    {
        bytes[i] = cursor->buf[i];
    }

    next_cursor();
    bytes += remaining;

    for (int i = 0; i < into_secondary; ++i)
    {
        bytes[i] = cursor->buf[i];
    }

    cursor->buf += into_secondary;
}

unsigned long ReadBuffer::read_ulong()
{
    // 00 00 00 00 00  EoF    00 00 00

    size_t remaining = sector_remaining();
    unsigned long rax;

    if (remaining < sizeof(unsigned long))
    {
        read_across_buffers(reinterpret_cast<char*>(&rax), remaining, sizeof(unsigned long));
        return rax;
    } else
    {
        rax = __builtin_bswap64(*reinterpret_cast<unsigned long*>(cursor->buf));
        cursor->buf += sizeof(unsigned long);
        return rax;
    }
}

long ReadBuffer::read_long()
{
    return std::bit_cast<long>(read_ulong());
}

unsigned short ReadBuffer::read_ushort()
{
    size_t remaining = sector_remaining();
    unsigned short rax;

    if (remaining < sizeof(unsigned short))
    {
        read_across_buffers(reinterpret_cast<char*>(&rax), remaining, sizeof(unsigned short));
        return rax;
    } else
    {
        rax = __builtin_bswap16(*reinterpret_cast<unsigned short*>(cursor->buf));
        cursor->buf += sizeof(unsigned short);
        return rax;
    }
}

std::string ReadBuffer::read_string()
{
    size_t remaining = sector_remaining();
    int size = read_varint();

    if (remaining < size)
    {
        std::string rax = std::string();
        rax.resize(size);
        read_across_buffers(const_cast<char*>(rax.c_str()), remaining, size);;
        return rax;
    } else
    {
        std::string rax = std::string(cursor->buf, size);
        cursor->buf += size;
        return rax;
    }
}

UUID ReadBuffer::read_uuid()
{
    return {read_ulong(), read_ulong()};
}

char** ReadBuffer::primary_end()
{
    return &iv->end;
}

char** ReadBuffer::secondary_end()
{
    return &iv[1].end;
}
