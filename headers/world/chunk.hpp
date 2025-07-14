//
// Created by cory on 1/15/25.
//

#ifndef CLAMS_CHUNK_HPP
#define CLAMS_CHUNK_HPP


#include <string>
#include "section.hpp"
#include "block_entities.hpp"
#include "heightmaps.hpp"
#include "structures.hpp"

class Chunk
{
private:
    std::string status;
    int32_t version;
    int32_t cx;
    int32_t cy;
    int32_t cz;
    int64_t last_update;
    int64_t inhabited_time;

    int32_t min_section;
    int32_t max_section;

    int32_t sections_len;
    ChunkSection* sections;
    BlockEntities tile_entities;
    Heightmaps heightmaps;
    Structures structures;
};


#endif //CLAMS_CHUNK_HPP
