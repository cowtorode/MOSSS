//
// Created by cory on 4/11/25.
//

#ifndef CPPTEST_UUID_HPP
#define CPPTEST_UUID_HPP


#include <ostream>

class UUID
{
public:
    unsigned long most;
    unsigned long least;

    UUID(unsigned long most, unsigned long least);

    friend std::ostream& operator<<(std::ostream& os, UUID& uuid);
};


#endif //CPPTEST_UUID_HPP
