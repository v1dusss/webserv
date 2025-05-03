//
// Created by Emil Ebert on 30.04.25.
//

#include "Server.h"
#include "common/Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "ServerPool.h"
#include "requestHandler/RequestHandler.h"
#include "response/HttpResponse.h"

Server::Server(ServerConfig config) : config(config) {
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

    int opt = 1;
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

bool Server::listen(ServerPool *pool) {
    if (::listen(serverFd, SOMAXCONN) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to listen on socket");
        return false;
    }

    Logger::log(LogLevel::DEBUG, "Listening on fd: " + std::to_string(serverFd));

    pool->registerFdToServer(serverFd, this, POLLIN);
    return true;
}

void Server::handleFdEvent(int fd, ServerPool *pool, short events) {
    if (fd == serverFd && events & POLLIN) {
        handleNewConnections(pool);
        return;
    }

    if (clients.find(fd) == clients.end()) {
        Logger::log(LogLevel::ERROR, "Client fd not found in map");
        return;
    }

    ClientConnection &clientConnection = clients[fd];
    if (events & POLLIN)
        handleClientInput(clientConnection, pool);
    if (events & POLLOUT)
        handleClientOutput(clientConnection);
}

void Server::handleNewConnections(ServerPool *pool) {
    struct sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);
    const int clientFd = accept(serverFd, reinterpret_cast<struct sockaddr *>(&clientAddr), &addrLen);
    if (clientFd < 0) {
        Logger::log(LogLevel::ERROR, "Failed to accept client connection");
        return;
    }

    Logger::log(LogLevel::INFO, "Accepted new client connection");
    Logger::log(LogLevel::DEBUG, "Client fd: " + std::to_string(clientFd));
    clients[clientFd] = ClientConnection(clientFd, clientAddr);
    pool->registerFdToServer(clientFd, this, POLLIN | POLLOUT);
}

void Server::handleClientInput(ClientConnection &clientConnection, ServerPool *pool) {
    char buffer[1024];
    const ssize_t bytesRead = read(clientConnection.fd, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        Logger::log(LogLevel::ERROR, "Failed to read from client");
        return;
    }

    if (bytesRead == 0) {
        closeClientConnection(clientConnection, pool);
        return;
    }


    clientConnection.buffer += std::string(buffer, bytesRead);
    //std::cout << clientConnection.buffer << std::endl;
    //  std::cout << "-------------------------" << std::endl;

    if (clientConnection.parser.parse(buffer, bytesRead)) {
        auto request = clientConnection.parser.getRequest();

        Logger::log(LogLevel::INFO, "Request Parsed");
        request->printRequest();
        std::cout << std::endl;

        RequestHandler requestHandler(clientConnection, *request, config);
        HttpResponse response = requestHandler.handleRequest();

        clientConnection.setResponse(response.toString());
        clientConnection.parser.reset();
        clientConnection.buffer.clear();

        std::string connectionHeader = request->getHeader("Connection");
        if (connectionHeader == "close") {
            closeClientConnection(clientConnection, pool);
        } else if (connectionHeader != "keep-alive") {
            clientConnection.setResponse(Response::customResponse(
                HttpResponse::StatusCode::BAD_REQUEST, "Bad Request, connection type is not supported").toString());
            closeClientConnection(clientConnection, pool);
        }
        return;
    }

    if (clientConnection.parser.hasError()) {
        std::cout << "Error in parser" << std::endl;
        std::cout << clientConnection.buffer << std::endl;

        clientConnection.setResponse(Response::customResponse(
            HttpResponse::StatusCode::BAD_REQUEST, "Bad Request").toString());
        clientConnection.parser.reset();
        closeClientConnection(clientConnection, pool);
    }
}

void Server::handleClientOutput(ClientConnection &client) {
    if (client.hasPendingResponse()) {
        const std::string response = client.getResponse();
        const size_t bytesWritten = write(client.fd, response.c_str(), response.size());

        if (bytesWritten < response.size()) {
            client.setResponse(response.substr(bytesWritten));
            return;
        }

        client.clearResponse();
    }
}

void Server::closeClientConnection(const ClientConnection &client, ServerPool *pool) {
    pool->unregisterFdFromServer(client.fd);
    close(client.fd);
    clients.erase(client.fd);
    Logger::log(LogLevel::INFO, "Closed client connection");
}


void Server::stop() {
    if (serverFd >= 0) {
        for (const auto &[fd, _]: clients) {
            close(fd);
            Logger::log(LogLevel::DEBUG, "closed client fd: " + std::to_string(fd));
        }
        clients.clear();

        Logger::log(LogLevel::DEBUG, "Stopping server on fd: " + std::to_string(serverFd));
        close(serverFd);
        serverFd = -1;
        Logger::log(LogLevel::INFO, "Server stopped");
    }
}
