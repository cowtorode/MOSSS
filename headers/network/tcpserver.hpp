//
// Created by cory on 1/15/25.
//

#ifndef CLAMS_TCPSERVER_HPP
#define CLAMS_TCPSERVER_HPP


#include <string>
#include "connection.hpp"

const std::string& get_public_key();

/**
 * Used to initialize server resources such as the server_fd and network workers.
 * @return true if initialization was good, false if it failed
 */
bool tcp_init(unsigned short port);

void tcp_start();

void tcp_stop();

void disconnect(Connection* conn);

void disconnect_all();


#endif //CLAMS_TCPSERVER_HPP
