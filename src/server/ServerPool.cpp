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

ServerPool::ServerPool() : running(false) {
}

ServerPool::~ServerPool() {
    stop();
}

void ServerPool::loadConfig(const std::string &configFile) {
    Logger::log(LogLevel::INFO, "Loading configuration from file: " + configFile);

    // TODO: Parse the configuration file and populate serverConfigs
    std::vector<ServerConfig> serverConfigs;
    ServerConfig testConfig;
    testConfig.port = 8080;
    testConfig.host = "localhost";
    testConfig.server_names.emplace_back("localhost");
    testConfig.root = "./www";

    RouteConfig routeConfig;
    routeConfig.allowedMethods.push_back(HttpMethod::GET);
    routeConfig.allowedMethods.push_back(HttpMethod::DELETE);
    routeConfig.allowedMethods.push_back(HttpMethod::POST);
    routeConfig.path = "/";
    routeConfig.cgi_params[".py"] = "/usr/bin/python3";
    routeConfig.autoindex = true;
    testConfig.routes.emplace_back(routeConfig);

    RouteConfig routeConfig2;
    routeConfig2.allowedMethods.push_back(HttpMethod::GET);
    routeConfig2.path = "/moin";
    routeConfig2.autoindex = true;
    routeConfig2.index = "index.html";
    routeConfig2.root = "./www/test";
    testConfig.routes.emplace_back(routeConfig2);

    serverConfigs.emplace_back(testConfig);

    for (const auto &server_config: serverConfigs) {
        auto server = std::make_shared<Server>(server_config);
        if (!server->createSocket()) {
            continue;
        }

        servers.emplace_back(server);
    }
}

void ServerPool::start() {
    if (running.load()) {
        std::cout << "Server pool is already running." << std::endl;
        return;
    }

    int startedServers = 0;
    for (const auto &server: servers) {
        if (server->listen(this)) {
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
        const int pollResult = poll(fds.data(), fds.size(), 0);
        if (pollResult < 0) {
            Logger::log(LogLevel::ERROR, "Poll error");
            Logger::log(LogLevel::ERROR,  strerror(errno));
            continue;
        }
        if (pollResult == 0) {
            continue;
        }

        for (const auto &fd: fds) {
            if (fd.revents & POLLIN || fd.revents & POLLOUT) {
                const auto server = serverFds[fd.fd];
                server->handleFdEvent(fd.fd, this, fd.revents);
            }
        }
    }

    cleanUp();
}

void ServerPool::registerFdToServer(const int fd, Server *server, const short events) {
    pollfd newFd{};
    newFd.fd = fd;
    newFd.events = events;
    fds.push_back(newFd);
    serverFds[fd] = server;
}

void ServerPool::unregisterFdFromServer(const int fd) {
    auto it = std::remove_if(fds.begin(), fds.end(), [fd](const pollfd &pfd) { return pfd.fd == fd; });
    if (it != fds.end()) {
        fds.erase(it, fds.end());
        serverFds.erase(fd);
    }
}

void ServerPool::cleanUp() {
    servers.clear();
    serverFds.clear();
    fds.clear();
    Logger::log(LogLevel::INFO, "Server pool cleaned up.");
}
