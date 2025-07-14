//
// Created by cory on 4/15/25.
//

#include <iostream>
#include "network/writebuffer.hpp"

WriteBuffer::WriteBuffer(unsigned long size) : _iov_size(1), _iov(new iovec[1]), iov_cursor(0), packet_length(0),
                                               buf(new char[size + 5]), end(buf + size + 5), sector_start(buf + 5), cursor(sector_start)
{}

WriteBuffer::~WriteBuffer()
{
    delete[] buf;
    delete[] _iov;
}

size_t WriteBuffer::buffer_size() const
{
    return end - buf;
}

size_t WriteBuffer::utilized_size() const
{
    return cursor - buf;
}

size_t WriteBuffer::sector_size() const
{
    return cursor - sector_start;
}

ptrdiff_t WriteBuffer::sector_remaining() const
{
    return end - cursor;
}

void WriteBuffer::buffer_resize(size_t size)
{
    char* resized = new char[size];
    size_t to_copy = utilized_size();

    if (size < to_copy)
    {
        // if the new size is less than the number of bytes we need to copy,
        // we're not going to copy what doesn't fit in the new buf
        to_copy = size;
    }

    for (ptrdiff_t i = 0; i < to_copy; ++i)
    {
        // copy the current buf to the new one
        resized[i] = buf[i];
    }

    end = resized + size;
    sector_start = sector_start - buf + resized;
    cursor = cursor - buf + resized;
    delete[] buf;
    buf = resized;
}

#define EXTRA_CAPACITY (32)

void WriteBuffer::ensure_capacity(size_t bytes)
{
    if (sector_remaining() < bytes)
    {
        // todo: figure out something better for resizing?
        buffer_resize(buffer_size() + bytes + EXTRA_CAPACITY);
    }
}

void WriteBuffer::iov_resize(int size)
{
    // todo check for max iovec size
    auto* resized = new iovec[size];

    for (int i = 0; i < _iov_size; ++i)
    {
        resized[i] = _iov[i];
    }

    delete[] _iov;

    _iov_size = size;
    _iov = resized;
}

#define WRITE(type, x) ensure_capacity(sizeof(type)); *reinterpret_cast<type*>(cursor) = x; cursor += sizeof(type);

void WriteBuffer::write_byte(int8_t x)
{
    WRITE(int8_t, x)
}

void WriteBuffer::write_bool(bool x)
{
    // This is implemented this way since bool isn't guaranteed to be 1 byte in size
    write_byte(x ? 1 : 0);
}

void WriteBuffer::write_short_le(int16_t x)
{
    WRITE(int16_t, x)
}

void WriteBuffer::write_short(int16_t x)
{
    write_short_le(std::bit_cast<int16_t>(__builtin_bswap16(x)));
}

void WriteBuffer::write_int_le(int32_t x)
{
    WRITE(int32_t, x)
}

void WriteBuffer::write_int(int32_t x)
{
    write_int_le(std::bit_cast<int32_t>(__builtin_bswap32(x)));
}

void WriteBuffer::write_long_le(int64_t x)
{
    WRITE(int64_t, x)
}

void WriteBuffer::write_long(int64_t x)
{
    write_long_le(std::bit_cast<int64_t>(__builtin_bswap64(x)));
}

void WriteBuffer::write_float_le(float x)
{
    WRITE(float, x)
}

void WriteBuffer::write_float(float x)
{
    write_float_le(std::bit_cast<float>(__builtin_bswap32(*reinterpret_cast<int32_t*>(&x))));
}

void WriteBuffer::write_double_le(double x)
{
    WRITE(double, x)
}

void WriteBuffer::write_double(double x)
{
    write_double_le(std::bit_cast<double>(__builtin_bswap64(*reinterpret_cast<int64_t*>(&x))));
}

#define WRITE_VARINT(type, x)                             \
    while (true)                                          \
    {                                                     \
        if ((x & ~0x7f) == 0)                             \
        {                                                 \
            write_byte(static_cast<char>(x));             \
            return;                                       \
        }                                                 \
        write_byte(static_cast<char>((x & 0x7f) | 0x80)); \
        x >>= 7;                                          \
    }                                                     \

void WriteBuffer::write_varint(int32_t x)
{
    WRITE_VARINT(int32_t, x)
}

void WriteBuffer::write_varlong(int64_t x)
{
    WRITE_VARINT(int64_t, x)
}

void WriteBuffer::write_uuid(const UUID& uuid)
{
    ensure_capacity(sizeof(UUID));

    *reinterpret_cast<uint64_t*>(cursor) = __builtin_bswap64(uuid.most);
    cursor += sizeof(uint64_t);
    *reinterpret_cast<uint64_t*>(cursor) = __builtin_bswap64(uuid.least);
    cursor += sizeof(uint64_t);
}

void WriteBuffer::write_iov(char* bytes, size_t size)
{
    packet_length += static_cast<int>(size);

    iovec* vec = _iov + iov_cursor;
    vec->iov_base = bytes;
    vec->iov_len = size;
    // sector was written, we're looking at the next vector now
    ++iov_cursor;

    // create new sector
    sector_start = cursor;
}

void WriteBuffer::write_bytes(char* bytes, size_t size)
{
    if (size)
    {
        size_t sector_len = sector_size();
        bool sector_exists = sector_len > 0;

        // ensure the size is big enough for our two writes to the iovector
        if (iov_cursor >= _iov_size - sector_exists - 1)
        {
            // if there needs to be a resize to fit the incoming sector, lets do it
            iov_resize(iov_cursor + 2);
        }

        if (sector_exists)
        {
            write_iov(sector_start, sector_len);
        } else if (!iov_cursor)
        {
            // if there hasn't been any writes we HAVE to write the start of the sector in order for the packet length to be written
            // whenever the buf is finalized
            write_iov(sector_start, 0);
        }

        write_iov(bytes, size);
    }
}

void WriteBuffer::write_string(const std::string& str)
{
    write_varint(static_cast<int32_t>(str.size()));
    write_bytes(const_cast<char*>(str.c_str()), str.size());
}

void WriteBuffer::write_chunk(const Chunk& chunk)
{
    // Field Name 	            Field Type                      Notes
    // Heightmaps 	            Prefixed Array of Heightmap
    // Data 	                Prefixed Array of Byte
    // Block
    // Entities     Packed XZ 	                Unsigned Byte 	The packed section coordinates are relative to the chunk they are in. Values 0-15 are valid.
    //
    //                                                          packed_xz = ((blockX & 15) << 4) | (blockZ & 15) // encode
    //                                                          x = packed_xz >> 4, z = packed_xz & 15 // decode
    //                          Prefixed Array
    //              Y 	                        Short 	        The height relative to the world
    //              Type 	                    VarInt      	The type of block entity
    //              Data 	                    NBT 	        The block entity's data, without the X, Y, and Z values
    //https://minecraft.wiki/w/Java_Edition_protocol/Packets#Chunk_Data
}

bool WriteBuffer::flush_buffer()
{
    size_t sector_len = sector_size();

    if (sector_len > 0)
    {
        // if the buf is not empty
        // resize _iov if necessary
        if (iov_cursor >= _iov_size - 1)
        {
            // if there needs to be a resize to fit the incoming sector, lets do it
            iov_resize(iov_cursor + 1);
        }

        write_iov(sector_start, sector_len);
        return true;
    }
    return false;
}

iovec* WriteBuffer::iov()
{
    if (flush_buffer() || iov_cursor)
    {
        // if the buf was flushed, we can guarantee at least one iovec present

        // write _len to len
        char len[5];
        char* curs = len;
        int x = packet_length;

        while (true)
        {
            if ((x & ~0x7f) == 0)
            {
                *curs++ = static_cast<char>(x);
                break;
            }
            *curs++ = static_cast<char>((x & 0x7f) | 0x80);
            x >>= 7;
        }

        // length of the length
        // we need to rewrite the first sector pointer to account for the length of the length that's getting written
        long len_length = curs - len;

        char* start = reinterpret_cast<char*>(_iov[0].iov_base) - len_length;
        _iov[0].iov_base = start;
        _iov[0].iov_len += len_length;

        // copy the written length from len to the start of the buf
        for (int i = 0; i < len_length; ++i)
        {
            start[i] = len[i];
        }

//        size_t printed = 0;
//
//        std::cout << std::hex;
//        for (int i = 0; i < iov_cursor; ++i)
//        {
//            for (size_t j = 0; j < _iov[i].iov_len; ++j)
//            {
//                if (printed % 16 == 0 && printed > 0)
//                {
//                    std::cout << std::endl;
//                }
//
//                std::cout << (static_cast<unsigned int>(reinterpret_cast<char*>(_iov[i].iov_base)[j]) & 0xff) << " ";
//                ++printed;
//            }
//        }
//        std::cout << std::dec << std::endl;
    }

    return _iov;
}

void WriteBuffer::reset()
{
    sector_start = buf + 5;
    cursor = sector_start;
    iov_cursor = 0;
    packet_length = 0;
}
