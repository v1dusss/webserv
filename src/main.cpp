//
// Created by Emil Ebert on 30.04.25.
//

#include <iostream>
#include "server/ServerPool.h"
#include <csignal>

#include "common/Logger.h"

ServerPool serverPool;

void signalHandler(int signum) {
    std::cout << std::endl;
    serverPool.stop();
    (void) signum;
}

void setupSignalHandler() {
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sa.sa_flags = SA_RESTART;  // Auto-restart interrupted system calls
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        Logger::log(LogLevel::ERROR, "Failed to set up signal handler");
        exit(1);
    }
}

int main() {
    setupSignalHandler();  // Instead of signal(SIGINT, signalHandler);

    serverPool.loadConfig("config.json");
    serverPool.start();
}