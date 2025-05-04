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
    int serverFd;
    ServerConfig config;
    std::unordered_map<int, ClientConnection> clients;

public:
    Server(ServerConfig config);

    ~Server();

    bool createSocket();

    bool listen(ServerPool *pool);

    void stop();

    void handleFdEvent(int fd, ServerPool *pool, short events);

    void closeConnections(ServerPool *pool);

    int getFd() const { return serverFd; }

private:
    void handleNewConnections(ServerPool *pool);

    void handleClientInput(ClientConnection &client, ServerPool *pool);

    void handleClientOutput(ClientConnection &client, ServerPool *pool);

    void closeClientConnection(const ClientConnection &client, ServerPool *pool);
};


#endif //SERVER_H
