//
// Created by cory on 4/15/25.
//

#ifndef CPPTEST_WRITEBUFFER_HPP
#define CPPTEST_WRITEBUFFER_HPP


#include <string>
#include <bits/types/struct_iovec.h>
#include "math/uuid.hpp"
#include "world/chunk.hpp"

class WriteBuffer
{
public:
    explicit WriteBuffer(unsigned long size);

    ~WriteBuffer();

    void write_byte(int8_t x);

    void write_bool(bool x);

    /**
     * Writes a signed 16-bit little-endian integer (defined for little-endian systems.)
     */
    void write_short_le(int16_t x);

    /**
     * Writes a signed 16-bit big-endian integer (defined for little-endian systems.)
     */
    void write_short(int16_t x);

    /**
     * Writes a signed 32-bit little-endian integer (defined for little-endian systems.)
     */
    void write_int_le(int32_t x);

    /**
     * Writes a signed 32-bit big-endian integer (defined for little-endian systems.)
     */
    void write_int(int32_t x);

    void write_long_le(int64_t x);

    void write_long(int64_t x);

    void write_float_le(float x);

    void write_float(float x);

    void write_double_le(double x);

    void write_double(double x);

    void write_varint(int32_t x);

    void write_varlong(int64_t x);

    void write_uuid(const UUID& uuid);

    void write_bytes(char* bytes, size_t size);

    void write_string(const std::string& str);

    void write_chunk(const Chunk& chunk);

    [[nodiscard]] inline int iov_size() const noexcept { return iov_cursor; }

    iovec* iov();

    /**
     * Resets the write buf to its initial state by resetting the sector_start and cursor
     * to the beginning of the buf, and resetting the iov_cursor to 0.
     *
     * This is useful for reusing a write buf for multiple writes.
     *
     * @note This does not free the memory allocated for the buf, it only resets the
     *       internal state of the buf.
     *
     * @note This does not reset the internal state of the iovector, it only resets the
     *       internal state of the buf.
     */
    void reset();
private:
    /**
     * @return Measurement of how many bytes are allocated in the byte buf.
     */
    [[nodiscard]] inline size_t buffer_size() const;

    /**
     * @return Measurement of how many bytes are written to.
     */
    [[nodiscard]] inline size_t utilized_size() const;

    /**
     * @return Measurement of how many bytes are in the current sector.
     */
    [[nodiscard]] inline size_t sector_size() const;

    /**
     * @return Measurement of how many bytes the write cursor has until it runs out of space,
     *         computed as the distance between the cursor and the end of the buf.
     */
    [[nodiscard]] inline ptrdiff_t sector_remaining() const;

    /**
     * Writes the current sector to the _iov at iov_cursor, dynamically resizing the _iov
     * if necessary to fill it. This also establishes a new sector for future writes.
     * @return true if flushed, false if nothing to flush.
     */
    bool flush_buffer();

    /**
     * Writes the given bytes of the given size to the iov at the current iov_cursor position.
     */
    void write_iov(char* bytes, size_t size);

    void ensure_capacity(size_t bytes);

    /**
     * @param size should be positive
     */
    void buffer_resize(size_t size);

    void iov_resize(int size);

    int _iov_size;
    iovec* _iov;
    int iov_cursor;

    /**
     * Used for writing the packet length after the entire packet has been written
     */
    int packet_length;

    /**
     * Pointer to the beginning of the allocated buf owned by this write
     * buf. All internally stored data is written into this memory, and
     * it is freed when the buf is destroyed.
     *
     * This should only change if the buf is resized for any reason.
     */
    char* buf;
    /**
     * Points to one past the last byte in the internal buf (buf + size).
     * Used to ensure that writes do not overflow the allocated memory region.
     *
     * This should also only be changed if the buf is resized for any reason,
     * otherwise it is to remain constant.
     */
    char* end;
    /**
     * Marks the start of the current sector in the internal buf. Each iovec
     * entry that refers to internal storage begins from this pointer, and spans
     * up to the current cursor position at the time the sector is closed.
     */
    char* sector_start;
    /**
     * Pointer to the current write position within the internal buf. As
     * owned data is written, this advances forward. May be used to construct
     * iovec entries for buf-backed writes.
     */
    char* cursor;
};


#endif //CPPTEST_WRITEBUFFER_HPP
