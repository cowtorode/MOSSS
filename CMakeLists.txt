cmake_minimum_required(VERSION 3.30)
project(CLAMS)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(headers)

# Define the executable target
add_executable(CLAMS src/main.cpp
        src/chunk_cache.cpp
        headers/chunk_cache.hpp
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
        src/chunk.cpp
        headers/chunk.hpp
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
)

# Link the target with liburing
target_link_libraries(CLAMS PRIVATE OpenSSL::SSL)
