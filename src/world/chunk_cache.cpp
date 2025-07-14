//
// Created by cory on 1/15/25.
//

#include <iostream>
#include "world/chunk_cache.hpp"

Chunk *ChunkCache::at(int cx, int cz) const noexcept
{
    return nullptr;
}

void ChunkCache::request_chunks(Player *player, int distance) const
{
    Position pos = player->getPosition();
    int cx = pos.cx();
    int cz = pos.cz();
    int dist;
    Chunk* chunk;

    bool debug[2 * distance + 1][2 * distance + 1];

    for (int i = 0; i < 2 * distance + 1; ++i)
    {
        for (int j = 0; j < 2 * distance + 1; ++j)
        {
            debug[i][j] = false;
        }
    }

    for (int i = -distance; i <= distance; ++i)
    {
        dist = distance - /*abs(i)*/(i < 0 ? -i : i);
        for (int j = -dist; j <= dist; ++j)
        {
            chunk = at(cx + i, cz + j);

            if (!chunk)
            {
                //std::cout << "Loading [cx + " << i << ", cz + " << j << "]" << std::endl;
                debug[i + distance][j + distance] = true;
                // register async Chunk send callback
            } else
            {
                std::cout << "Sending chunk at [cx + " << i << ", cz + " << j << "] to player" << std::endl;
                // fixme: async?
                //player->send(chunk);
            }
        }
    }

    for (int i = 0; i < 2 * distance + 1; ++i)
    {
        for (int j = 0; j < 2 * distance + 1; ++j)
        {
            std::cout << (debug[i][j] ? '@' : '.') << "  ";
        }
        std::cout << std::endl;
    }
}
