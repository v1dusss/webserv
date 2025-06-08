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
    static std::vector<std::shared_ptr<Server> > servers;
    static std::atomic<bool> running;
    static std::unordered_map<int, std::shared_ptr<ClientConnection> > clients;
    static std::vector<ServerConfig> configs;

public:
    static void registerClient(int clientFd, const sockaddr_in &clientAddr, const Server *connectedServer);

    static bool loadConfig(const std::string &configFile);

    static void matchVirtualServer(ClientConnection *client, const std::string &hostHeader);

    static void start();

    static void stop();

private:
    static void serverLoop();

    static void cleanUp();

    static void closeConnections();

};


#endif //SERVERPOOL_H
