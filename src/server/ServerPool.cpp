//
// Created by Emil Ebert on 01.05.25.
//

#include "ServerPool.h"
#include "common/Logger.h"

#include <iostream>
#include <unistd.h>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <parser/config/ConfigParser.h>

#include "FdHandler.h"

ServerPool::ServerPool() : running(false) {
}

ServerPool::~ServerPool() {
    stop();
}

bool ServerPool::loadConfig(const std::string &configFile) {
    Logger::log(LogLevel::INFO, "Loading configuration from file: " + configFile);

    ConfigParser parser;

    if (!parser.parse(configFile)) {
        Logger::log(LogLevel::ERROR, "Failed to parse configuration file: " + configFile);
        return false;
    }

    const std::vector<ServerConfig> serverConfigs = parser.getServerConfigs();

    for (const auto &server_config: serverConfigs) {
        auto server = std::make_shared<Server>(server_config);
        if (!server->createSocket()) {
            continue;
        }

        servers.emplace_back(server);
    }
    return true;
}

void ServerPool::start() {
    if (running.load()) {
        std::cout << "Server pool is already running." << std::endl;
        return;
    }

    int startedServers = 0;
    for (const auto &server: servers) {
        if (server->listen()) {
            startedServers++;
        }
    }

    Logger::log(LogLevel::INFO, "Server pool started.");
    Logger::log(LogLevel::INFO,
                std::to_string(startedServers) + " servers started from " + std::to_string(servers.size()) +
                " configured servers.");
    running.store(true);
    serverLoop();
}

void ServerPool::stop() {
    running.store(false);
}

void ServerPool::serverLoop() {
    while (running.load()) {
        for (const auto &shared_ptr: servers)
            shared_ptr->closeConnections();
        FdHandler::pollFds();
    }
    cleanUp();
}

void ServerPool::cleanUp() {
    servers.clear();
    Logger::log(LogLevel::INFO, "Server pool cleaned up.");
}
