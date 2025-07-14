//
// Created by cory on 4/11/25.
//

#ifndef CPPTEST_UUID_HPP
#define CPPTEST_UUID_HPP


#include <ostream>
#include <cstdint>

class UUID
{
public:
    uint64_t most;
    uint64_t least;

    UUID(uint64_t most, uint64_t least);

    friend std::ostream& operator<<(std::ostream& os, UUID& uuid);
};


#endif //CPPTEST_UUID_HPP
