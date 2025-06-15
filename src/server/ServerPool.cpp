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

#include "handler/CallbackHandler.h"
#include "FdHandler.h"
#include "handler/MetricHandler.h"

std::vector<std::shared_ptr<Server> > ServerPool::servers;
std::atomic<bool> ServerPool::running{false};
std::unordered_map<int, std::shared_ptr<ClientConnection> > ServerPool::clients;
std::vector<ServerConfig> ServerPool::configs;
std::time_t ServerPool::startTime = 0;
HttpConfig ServerPool::httpConfig;

void ServerPool::registerClient(int clientFd, const sockaddr_in &clientAddr, const Server *connectedServer) {
    clients[clientFd] = std::make_shared<ClientConnection>(clientFd, clientAddr, connectedServer);
}

void ServerPool::matchVirtualServer(ClientConnection *client, const std::string &hostHeader) {
    if (!client || !client->connectedServer) {
        return;
    }

    std::string hostname = hostHeader;
    size_t colonPos = hostname.find(':');
    if (colonPos != std::string::npos) {
        hostname = hostname.substr(0, colonPos);
    }

    const int port = client->connectedServer->getPort();
    const std::string connectedHost = client->connectedServer->getHost();

    ServerConfig *defaultServer = nullptr;
    ServerConfig *wildcardPrefixMatch = nullptr;
    ServerConfig *wildcardSuffixMatch = nullptr;
    size_t longestPrefixMatch = 0;
    size_t longestSuffixMatch = 0;

    for (auto &serverConfig: configs) {
        if (serverConfig.port != port ||
            (connectedHost != "0.0.0.0" && serverConfig.host != connectedHost && serverConfig.host != "0.0.0.0")) {
            continue;
        }

        if (!defaultServer)
            defaultServer = &serverConfig;

        for (const std::string &serverName: serverConfig.server_names) {
            if (serverName == hostname) {
                client->setConfig(serverConfig);
                return;
            }

            if (serverName.size() > 2 && serverName[0] == '*' && serverName[1] == '.') {
                std::string suffix = serverName.substr(1);
                if (hostname.size() >= suffix.size() &&
                    hostname.compare(hostname.size() - suffix.size(), suffix.size(), suffix) == 0) {
                    if (suffix.size() > longestPrefixMatch) {
                        longestPrefixMatch = suffix.size();
                        wildcardPrefixMatch = &serverConfig;
                    }
                }
            }

            if (serverName.size() > 2 && serverName.back() == '*' && serverName[serverName.size() - 2] == '.') {
                std::string prefix = serverName.substr(0, serverName.size() - 1);
                if (hostname.size() >= prefix.size() &&
                    hostname.compare(0, prefix.size(), prefix) == 0) {
                    if (prefix.size() > longestSuffixMatch) {
                        longestSuffixMatch = prefix.size();
                        wildcardSuffixMatch = &serverConfig;
                    }
                }
            }
        }
    }

    if (wildcardPrefixMatch) {
        client->setConfig(*wildcardPrefixMatch);
        std::cout << "Matched wildcard prefix: " << wildcardPrefixMatch->host << std::endl;
    } else if (wildcardSuffixMatch) {
        client->setConfig(*wildcardSuffixMatch);
        std::cout << "Matched wildcard suffix: " << wildcardSuffixMatch->host << std::endl;
    } else if (defaultServer) {
        client->setConfig(*defaultServer);
        std::cout << "Using default server: " << defaultServer->host << std::endl;
    }
}

bool ServerPool::loadConfig(const std::string &configFile) {
    Logger::log(LogLevel::INFO, "Loading configuration from file: " + configFile);

    ConfigParser parser;

    if (!parser.parse(configFile)) {
        Logger::log(LogLevel::ERROR, "Failed to parse configuration file: " + configFile);
        return false;
    }

    httpConfig = parser.getHttpConfig();
    configs = parser.getServerConfigs();

    for (const auto &serverConfig: configs) {
        auto server = std::make_shared<Server>(serverConfig.port, serverConfig.host, serverConfig);
        if (!server->createSocket()) {
            continue;
        }

        Logger::log(LogLevel::DEBUG, "Socket created on port: " + std::to_string(serverConfig.port) +
                                     " with host: " + serverConfig.host);
        servers.emplace_back(server);
    }
    return true;
}

void ServerPool::start() {
    if (running.load()) {
        std::cout << "Server pool is already running." << std::endl;
        return;
    }

    startTime = std::time(nullptr);

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
    startTime = 0;
}

void ServerPool::serverLoop() {
    while (running.load()) {
        closeConnections();
        FdHandler::pollFds();
        CallbackHandler::executeCallbacks();
        MetricHandler::resetMetrics();
    }
    cleanUp();
}

void ServerPool::closeConnections() {
    const time_t currentTime = std::time(nullptr);
    std::vector<int> clientsToClose;
    for (auto &[fd, client]: clients) {
        if (client->shouldClose) {
            clientsToClose.push_back(fd);
            continue;
        }

        if (client->keepAlive && !client->hasPendingResponse() && client->lastPackageSend != 0 &&
            currentTime - client->lastPackageSend > static_cast<long>(client->config.keepalive_timeout)) {
            clientsToClose.push_back(fd);
            Logger::log(LogLevel::INFO, "Client connection timed out");
            continue;
        }

        if (client->parser.headerStart != 0 &&
            currentTime - client->parser.headerStart > static_cast<long>(client->config.headerConfig.
                client_header_timeout)) {
            clientsToClose.push_back(fd);
            Logger::log(LogLevel::INFO, "Client connection header timed out");
            continue;
        }

        if (client->parser.getState() == ParseState::BODY && client->parser.bodyStart != 0 &&
            currentTime - client->parser.bodyStart > static_cast<long>(client->config.client_body_timeout)) {
            clientsToClose.push_back(fd);
            Logger::log(LogLevel::INFO, "Client connection body timed out");
        }
    }

    for (int fd: clientsToClose) {
        if (clients.find(fd) != clients.end()) {
            Logger::log(LogLevel::INFO, "Closed client connection");
            clients.erase(fd);
        }
    }
}

void ServerPool::cleanUp() {
    clients.clear();
    configs.clear();
    servers.clear();
    Logger::log(LogLevel::INFO, "Server pool cleaned up.");
}

int ServerPool::getClientCount() {
    return clients.size();
}

std::time_t ServerPool::getStartTime() {
    return startTime;
}

HttpConfig &ServerPool::getHttpConfig() {
    return httpConfig;
}
