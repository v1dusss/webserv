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
    static std::vector<std::shared_ptr<Server>> servers;
    static std::atomic<bool> running;

public:
    ServerPool();

    ~ServerPool();


    bool loadConfig(const std::string &configFile);

    void start();

    void stop();

private:
    void serverLoop();

    void cleanUp();
};


#endif //SERVERPOOL_H
