//
// Created by cory on 1/15/25.
//

#ifndef CLAMS_ENTITY_HPP
#define CLAMS_ENTITY_HPP

#include "math/position.hpp"
#include "math/uuid.hpp"


class Entity
{
public:
    explicit Entity(UUID uuid);

    Position getPosition() { return pos; }
protected:
    Position pos;
    UUID uuid;
};


#endif //CLAMS_ENTITY_HPP
