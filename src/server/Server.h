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
    const int port;
    const std::string host;
    const ServerConfig config;

public:
    Server(int port, std::string host, const ServerConfig &config);

    ~Server();

    bool createSocket();

    [[nodiscard]] bool listen() const;

    void stop();

    void handleFdEvent(int fd, short events);

    [[nodiscard]] int getFd() const { return serverFd; }

    [[nodiscard]] int getPort() const { return port; }

    [[nodiscard]] const std::string &getHost() const { return host; }

    [[nodiscard]] const ServerConfig &getConfig() const { return config; }

private:
    void handleNewConnections() const;
};


#endif //SERVER_H
