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
    explicit Entity(UUID uid);

    Position getPosition() { return pos; }

    inline const UUID& uuid() const { return uid; }
protected:
    Position pos;
    UUID uid;
};


#endif //CLAMS_ENTITY_HPP
