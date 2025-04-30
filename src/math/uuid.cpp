//
// Created by cory on 4/11/25.
//

#include "math/uuid.hpp"

UUID::UUID(unsigned long most, unsigned long least) : most(most), least(least)
{}

std::ostream& operator<<(std::ostream &os, UUID &uuid)
{
    os << std::hex << uuid.most << uuid.least << std::dec; // fixme
    return os;
}
