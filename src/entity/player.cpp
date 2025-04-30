//
// Created by cory on 1/15/25.
//

#include "entity/player.hpp"

Player::Player(UUID uuid, std::string& username) : Entity(uuid), username(username)
{}

Player::~Player() = default;
