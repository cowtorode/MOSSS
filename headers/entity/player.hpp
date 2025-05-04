//
// Created by cory on 1/15/25.
//

#ifndef CLAMS_PLAYER_HPP
#define CLAMS_PLAYER_HPP


#include <string>
#include "entity.hpp"

enum GameMode
{
    SURVIVAL, CREATIVE, ADVENTURE, SPECTATOR
};

class Player : public Entity
{
public:
    Player(UUID& uuid, std::string& username);

    ~Player();

    [[nodiscard]] inline const std::string& username() const { return name; }
private:
    std::string name;
};


#endif //CLAMS_PLAYER_HPP
