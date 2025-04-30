//
// Created by cory on 4/11/25.
//

#ifndef CLAMS_READBUFFER_HPP
#define CLAMS_READBUFFER_HPP


#include <string>
#include "math/uuid.hpp"

class IncompleteBufferException : std::exception {};

class MalformedVarintException : std::exception {};

class BufferOverflowException : std::exception {};

class ReadBuffer
{
public:
    ReadBuffer();

    void feed(char* buffer, unsigned long size);

    char read_char();

    inline bool read_bool() { return read_char(); }

    unsigned long read_ulong();

    unsigned short read_ushort();

    UUID read_uuid();

    int read_varint();

    std::string read_string();
private:
    char* buf;
    char* end;
};


#endif //CLAMS_READBUFFER_HPP
