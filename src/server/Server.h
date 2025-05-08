//
// Created by Emil Ebert on 30.04.25.
//

#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <poll.h>
#include <unistd.h>

#include "config/config.h"
#include "ClientConnection.h"
#include <unordered_map>
#include <common/Logger.h>

class ServerPool;

class Server {
private:
    int serverFd{};
    ServerConfig config;
    std::unordered_map<int, std::shared_ptr<ClientConnection>> clients;

public:
    Server(const ServerConfig& config);

    ~Server();

    bool createSocket();

    bool listen();

    void stop();

    void handleFdEvent(int fd, short events);

    void closeConnections();

    int getFd() const { return serverFd; }

private:
    void handleNewConnections();
};


#endif //SERVER_H
