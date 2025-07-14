cmake_minimum_required(VERSION 3.30)
project(MOSSS)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Define the executable target
add_library(MOSSS example/main.cpp
        src/world/chunk_cache.cpp
        headers/world/chunk_cache.hpp
        src/entity/player.cpp
        headers/entity/player.hpp
        src/entity/entity.cpp
        headers/entity/entity.hpp
        src/math/vec3d.cpp
        headers/math/vec3d.hpp
        src/math/position.cpp
        headers/math/position.hpp
        src/math/uuid.cpp
        headers/math/uuid.hpp
        src/world/chunk.cpp
        headers/world/chunk.hpp
        src/network/tcpserver.cpp
        headers/network/tcpserver.hpp
        src/server.cpp
        headers/server.hpp
        src/network/worker.cpp
        headers/network/worker.hpp
        headers/clamsutil.hpp
        src/network/connection.cpp
        headers/network/connection.hpp
        src/network/readbuffer.cpp
        headers/network/readbuffer.hpp
        src/network/writebuffer.cpp
        headers/network/writebuffer.hpp
        headers/loggerimpl.hpp
        src/loggerimpl.cpp
        headers/status.hpp
        headers/logger.hpp
        headers/network/clientbound_packet_ids.hpp
        headers/network/ivec.hpp
        src/world/section.cpp
        headers/world/section.hpp
        headers/network/buffer_helper.hpp
)

# Include directories
target_include_directories(MOSSS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

add_subdirectory(example)

# Link the target with liburing
target_link_libraries(MOSSS PRIVATE OpenSSL::SSL)
