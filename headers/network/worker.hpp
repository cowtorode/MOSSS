//
// Created by cory on 1/16/25.
//

#ifndef CLAMS_WORKER_HPP
#define CLAMS_WORKER_HPP


#include <thread>
#include <unordered_map>
#include "liburing.h"
#include "connection.hpp"
#include "writebuffer.hpp"

class NetworkWorker
{
public:
    NetworkWorker();

    ~NetworkWorker();

    void start();

    /**
     * Assign a client to this worker and create them a Connection object.
     */
    [[nodiscard]] Connection* connect(int client_fd) const;
private:
    std::thread runner;
    /**
     * Epoll file descriptor
     */
    int epfd;

    void listen() const;
};


#endif //CLAMS_WORKER_HPP
