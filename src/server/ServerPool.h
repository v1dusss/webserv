//
// Created by Emil Ebert on 01.05.25.
//

#ifndef SERVERPOOL_H
#define SERVERPOOL_H

#include <string>
#include <vector>
#include "Server.h"
#include <unordered_map>
#include <atomic>
#include <memory>
#include <queue>

class ServerPool {
private:
    std::vector<std::shared_ptr<Server> > servers;
    std::unordered_map<int, Server *> serverFds;

    std::queue<std::pair<int, short>> newClients;

    std::atomic<bool> running;
    std::vector<struct pollfd> fds;

public:
    ServerPool();

    ~ServerPool();

    void registerFdToServer(int fd, Server *server, short events);

    void unregisterFdFromServer(int fd);

    bool loadConfig(const std::string &configFile);

    void start();

    void stop();

private:
    void serverLoop();

    void cleanUp();
};


#endif //SERVERPOOL_H
