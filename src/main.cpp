//
// Created by Emil Ebert on 30.04.25.
//

#include <iostream>
#include "server/ServerPool.h"
#include <csignal>
#include <webserv.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <server/FdHandler.h>
#include <unistd.h>
#include "common/Logger.h"

ServerPool serverPool;

static void signalHandler(const int signum) {
    serverPool.stop();
    (void) signum;
}

static void setupSignalHandler() {
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        Logger::log(LogLevel::ERROR, "Failed to set up signal handler");
        exit(1);
    }
}

static void createTempDir() {
    if (mkdir(TEMP_DIR_NAME, 0777) == -1 && errno != EEXIST) {
        Logger::log(LogLevel::ERROR, "Failed to create temporary directory: " + std::string(strerror(errno)));
        exit(1);
    }
}

int main(const int argc, char **argv) {
    if (argc < 2) {
        Logger::log(LogLevel::ERROR, "Not enough arguments");
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    //::signal(SIGPIPE, SIG_IGN);


    std::cout << "start pid: " << getpid() << std::endl;

    createTempDir();
    if (!serverPool.loadConfig(argv[1]))
        return 1;

    setupSignalHandler();
    serverPool.start();
    return 0;
}
