//
// Created by cory on 1/15/25.
//

#ifndef CLAMS_CHUNK_CACHE_HPP
#define CLAMS_CHUNK_CACHE_HPP


#include "entity/player.hpp"
#include "chunk.hpp"

class ChunkCache
{
public:
    Chunk* at(int cx, int cz) const noexcept;

    void request_chunks(Player* player, int distance) const;
};


#endif //CLAMS_CHUNK_CACHE_HPP
