//
// Created by Emil Ebert on 30.04.25.
//

#include "Server.h"
#include "common/Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "FdHandler.h"
#include "ServerPool.h"
#include "requestHandler/RequestHandler.h"
#include "response/HttpResponse.h"

Server::Server(const int port, std::string host, const ServerConfig &config) : port(port), host(host), config(config) {
    Logger::log(LogLevel::DEBUG, "Server created with config: " + host + ":" + std::to_string(port));
}

Server::~Server() {
    stop();
}

bool Server::createSocket() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        Logger::log(LogLevel::ERROR, "Failed to create socket");
        return false;
    }

    constexpr int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to set socket options");
        close(serverFd);
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = custom_inet_addr(host.c_str());

    if (bind(serverFd, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0)
        return false;

    return true;
}

bool Server::listen() const {
    if (::listen(serverFd, 5024) < 0) {
        Logger::log(LogLevel::DEBUG, "Failed to listen on socket");
        return false;
    }

    Logger::log(LogLevel::DEBUG, "Listening on fd: " + std::to_string(serverFd));

    FdHandler::addFd(serverFd, POLLIN, [this](const int fd, const short events) {
        (void) fd;
        (void) events;
        handleNewConnections();
        return false;
    });
    return true;
}

void Server::handleNewConnections() const {
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);
    const int clientFd = accept(serverFd, reinterpret_cast<struct sockaddr *>(&clientAddr), &addrLen);
    if (clientFd < 0) {
        Logger::log(LogLevel::ERROR, "Failed to accept client connection");
        return;
    }

    constexpr int opt = 1;
    setsockopt(clientFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Logger::log(LogLevel::INFO, "Accepted new client connection");
    Logger::log(LogLevel::DEBUG, "Client fd: " + std::to_string(clientFd));
    ServerPool::registerClient(clientFd, clientAddr, this);
}


void Server::stop() {
    if (serverFd >= 0) {
        Logger::log(LogLevel::DEBUG, "Stopping server on fd: " + std::to_string(serverFd));

        close(serverFd);
        serverFd = -1;
        Logger::log(LogLevel::DEBUG, "Server stopped");
    }
}

in_addr_t Server::custom_inet_addr(const char* ip_address) {
    if (!ip_address) return INADDR_NONE;

    unsigned int octets[4] = {0};
    int currentOctet = 0;
    int currentValue = 0;
    bool hasDigit = false;

    for (const char* ptr = ip_address; *ptr != '\0'; ++ptr) {
        if (*ptr >= '0' && *ptr <= '9') {
            currentValue = currentValue * 10 + (*ptr - '0');
            if (currentValue > 255) return INADDR_NONE;
            hasDigit = true;
        } else if (*ptr == '.') {
            if (!hasDigit || currentOctet >= 3) return INADDR_NONE;
            octets[currentOctet++] = currentValue;
            currentValue = 0;
            hasDigit = false;
        } else {
            return INADDR_NONE;
        }
    }

    if (!hasDigit || currentOctet != 3) return INADDR_NONE;
    octets[3] = currentValue;

    return htonl((octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3]);
}
