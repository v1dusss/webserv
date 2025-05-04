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
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        Logger::log(LogLevel::ERROR, "Failed to set up signal handler");
        exit(1);
    }
}

int main(const int argc, char **argv) {
    if (argc < 2) {
        Logger::log(LogLevel::ERROR, "Not enough arguments");
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    if (!serverPool.loadConfig(argv[1]))
        return 1;

    setupSignalHandler();
    serverPool.start();
    return 0;
}
