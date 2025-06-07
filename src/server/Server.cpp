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

Server::Server(const ServerConfig &config) : config(config) {
    Logger::log(LogLevel::INFO, "Server created with config: " + config.host + ":" + std::to_string(config.port));
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
    Logger::log(LogLevel::DEBUG, "Socket created with fd: " + std::to_string(serverFd));

    constexpr int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to set socket options");
        close(serverFd);
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(config.port);
    serverAddr.sin_addr.s_addr = inet_addr(config.host.c_str());


    if (bind(serverFd, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to bind socket");
        return false;
    }

    return true;
}

bool Server::listen() {
    if (::listen(serverFd, 5024) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to listen on socket");
        return false;
    }

    Logger::log(LogLevel::DEBUG, "Listening on fd: " + std::to_string(serverFd));

    FdHandler::addFd(serverFd, POLLIN, [this](const int fd, short events) {
        (void) fd;
        (void) events;
        handleNewConnections();
        return false;
    });
    return true;
}

void Server::handleNewConnections() {
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);
    const int clientFd = accept(serverFd, reinterpret_cast<struct sockaddr *>(&clientAddr), &addrLen);
    if (clientFd < 0) {
        Logger::log(LogLevel::ERROR, "Failed to accept client connection");
        return;
    }

    int opt = 1;
    setsockopt(clientFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Logger::log(LogLevel::INFO, "Accepted new client connection");
    Logger::log(LogLevel::DEBUG, "Client fd: " + std::to_string(clientFd));
    clients[clientFd] = std::make_shared<ClientConnection>(clientFd, clientAddr, config);
}

void Server::closeConnections() {
    const time_t currentTime = std::time(nullptr);
    std::vector<int> clientsToClose;
    for (auto &[fd, client]: clients) {
        if ((client->shouldClose) || client->requestCount > config.
            keepalive_requests) {
            clientsToClose.push_back(fd);
            continue;
        }

        if (client->keepAlive && !client->hasPendingResponse() && client->lastPackageSend != 0 &&
            currentTime - client->lastPackageSend > static_cast<long>(config.keepalive_timeout)) {
            clientsToClose.push_back(fd);
            Logger::log(LogLevel::INFO, "Client connection timed out");
            continue;
        }

        if (client->parser.headerStart != 0 &&
            currentTime - client->parser.headerStart > static_cast<long>(config.headerConfig.client_header_timeout)) {
            clientsToClose.push_back(fd);
            Logger::log(LogLevel::INFO, "Client connection header timed out");
            continue;
        }

        if (client->parser.getState() == ParseState::BODY && client->parser.bodyStart != 0 &&
            currentTime - client->parser.bodyStart > static_cast<long>(config.client_body_timeout)) {
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


void Server::stop() {
    if (serverFd >= 0) {
        Logger::log(LogLevel::DEBUG, "Stopping server on fd: " + std::to_string(serverFd));
        clients.clear();

        close(serverFd);
        serverFd = -1;
        Logger::log(LogLevel::INFO, "Server stopped");
    }
}
