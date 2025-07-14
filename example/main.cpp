// CLAMS Entry Point
// (C++-based Library for Asynchronous Minecraft Servers)

#include <csignal>
#include <cstdlib>
#include "server.hpp"

void run()
{
    server_start(25565);
}

void shutdown(int code)
{
    server_stop();
    exit(0);
}

int main(int argc, char** argv)
{
    // initialization
    signal(SIGTERM, shutdown);
    signal(SIGABRT, shutdown);

    run();
    return 0;
}
