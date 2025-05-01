//
// Created by Emil Ebert on 01.05.25.
//

#include "ServerPool.h"
#include "common/Logger.h"

#include <iostream>

ServerPool::ServerPool() : running(false) {
}

ServerPool::~ServerPool() {
    stop();
}

void ServerPool::loadConfig(const std::string &configFile) {
   Logger::log(LogLevel::INFO, "Loading configuration from file: " + configFile);
}

void ServerPool::start(void) {
    if (running.load()) {
        std::cout << "Server pool is already running." << std::endl;
        return;
    }
}

void ServerPool::stop(void) {
    if (!running.load()) {
        std::cout << "Server pool is not running." << std::endl;
        return;
    }

    running.store(false);
    std::cout << "Stopping server pool..." << std::endl;

}

void ServerPool::serverLoop(void) {
    while (running.load()) {
        for (auto &fd : fds) {
            if (fd.revents & POLLIN) {
            }
        }
    }
}

