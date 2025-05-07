//
// Created by Emil Ebert on 30.04.25.
//

#include "Server.h"
#include "common/Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <webserv.h>
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

bool Server::listen(ServerPool *pool) {
    if (::listen(serverFd, SOMAXCONN) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to listen on socket");
        return false;
    }

    Logger::log(LogLevel::DEBUG, "Listening on fd: " + std::to_string(serverFd));

    pool->registerFdToServer(serverFd, this, POLLIN);
    return true;
}

void Server::handleFdEvent(const int fd, ServerPool *pool, const short events) {
    if (fd == serverFd && events & POLLIN) {
        handleNewConnections(pool);
        return;
    }

    if (clients.count(fd) == 0) {
        Logger::log(LogLevel::ERROR, "Client fd not found in map");
        return;
    }

    const std::shared_ptr<ClientConnection> &clientConnection = clients[fd];
    if (events & POLLIN && !clientConnection->shouldClose)
        handleClientInput(clientConnection, pool);
    if (events & POLLOUT)
        handleClientOutput(clientConnection, pool);
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
    clients[clientFd] = std::make_shared<ClientConnection>(clientFd, clientAddr);
    clients[clientFd]->parser.setClientLimits(config.client_max_body_size, config.client_max_header_size);


    pool->registerFdToServer(clientFd, this, POLLIN | POLLOUT);
}

void Server::handleClientInput(std::shared_ptr<ClientConnection> clientConnection, ServerPool *pool) {
    (void) pool;
    char buffer[1024];
    const ssize_t bytesRead = read(clientConnection->fd, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        Logger::log(LogLevel::ERROR, "Failed to read from client");
        return;
    }

    if (bytesRead == 0) {
        clientConnection->shouldClose = true;
        return;
    }
    clientConnection->buffer += std::string(buffer, bytesRead);
    //std::cout << clientConnection.buffer << std::endl;
    //  std::cout << "-------------------------" << std::endl;

    if (clientConnection->parser.parse(buffer, bytesRead)) {
        auto request = clientConnection->parser.getRequest();
        clientConnection->keepAlive = request->getHeader("Connection") == "keep-alive";

        Logger::log(LogLevel::INFO, "Request Parsed");
        //  request->printRequest();

        RequestHandler requestHandler(clientConnection, *request, config);
        const HttpResponse response = requestHandler.handleRequest();

        clientConnection->setResponse(response);
        return;
    }

    if (clientConnection->parser.hasError()) {
        std::cout << clientConnection->buffer << std::endl;

        const HttpResponse response = HttpResponse::html(HttpResponse::StatusCode::BAD_REQUEST);
        clientConnection->setResponse(RequestHandler::handleCustomErrorPage(response, config, std::nullopt));
    }
}

void Server::handleClientOutput(std::shared_ptr<ClientConnection> client, const ServerPool *pool) {
    (void) pool;
    if (client->hasPendingResponse()) {
        HttpResponse &response = client->getResponse().value();
        if (client->keepAlive)
            response.setHeader("Connection", "keep-alive");
        else
            response.setHeader("Connection", "close");

        if (response.isChunkedEncoding()) {
            handleClientFileOutput(client, response);
            return;
        }
        const std::string responseBuffer = response.toString();
        const size_t bytesWritten = write(client->fd, responseBuffer.c_str(), responseBuffer.size());

        if (bytesWritten < responseBuffer.size()) {
            Logger::log(LogLevel::ERROR, "Failed to write full response to client");
            return;
        }

        client->lastPackageSend = std::time(nullptr);
        Logger::log(LogLevel::INFO, "Client response sent");
        client->clearResponse();
    }
}

void Server::handleClientFileOutput(const std::shared_ptr<ClientConnection> &client, HttpResponse &response) const {
    if (!response.alreadySendHeader) {
        const std::string header = response.toHeaderString();
        std::cout << header << std::endl;
        if (write(client->fd, header.c_str(), header.length()) < 0) {
            Logger::log(LogLevel::ERROR, "Failed to write header to client");
            client->clearResponse();
            return;
        }
        response.alreadySendHeader = true;
    }

    const int bodyFd = response.getBodyFd();
    char buffer[config.body_buffer_size];

    pollfd pollfd{};
    pollfd.fd = bodyFd;
    pollfd.events = POLLIN;

    const int pollResult = poll(&pollfd, 1, READ_FILE_TIMEOUT);
    if (pollResult < 0) {
        Logger::log(LogLevel::ERROR, "Poll error while reading from client");
        client->clearResponse();
        return;
    }

    if (pollResult == 0) {
        Logger::log(LogLevel::ERROR, "Timeout while reading from client");
        client->clearResponse();
        return;
    }

    const ssize_t bytesRead = read(bodyFd, buffer, sizeof(buffer));

    if (bytesRead < 0) {
        Logger::log(LogLevel::ERROR, "Failed to read from file descriptor");
        client->clearResponse();
        return;
    }

    if (bytesRead == 0) {
        if (write(client->fd, "0\r\n\r\n", 5) < 0)
            Logger::log(LogLevel::ERROR, "Failed to write final chunk to client");
        client->lastPackageSend = std::time(nullptr);
        Logger::log(LogLevel::INFO, "Client response sent");
        client->clearResponse();
        return;
    }

    std::stringstream chunkHeader;
    chunkHeader << std::hex << bytesRead << "\r\n";
    const std::string header = chunkHeader.str();

    // Write the chunk header
    if (write(client->fd, header.c_str(), header.length()) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to write chunk header to client");
        client->clearResponse();
    }

    // Write the chunk data
    if (write(client->fd, buffer, bytesRead) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to write chunk data to client");
        client->clearResponse();
    }

    // Write the trailing CRLF
    if (write(client->fd, "\r\n", 2) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to write chunk trailing CRLF to client");
        client->clearResponse();
    }
}

void Server::closeConnections(ServerPool *pool) {
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
            currentTime - client->parser.headerStart > static_cast<long>(config.client_header_timeout)) {
            clientsToClose.push_back(fd);
            Logger::log(LogLevel::INFO, "Client connection header timed out");
            continue;
        }

        if (client->parser.bodyStart != 0 &&
            currentTime - client->parser.bodyStart > static_cast<long>(config.client_body_timeout)) {
            clientsToClose.push_back(fd);
            Logger::log(LogLevel::INFO, "Client connection body timed out");
        }
    }

    for (int fd: clientsToClose) {
        if (clients.find(fd) != clients.end()) {
            pool->unregisterFdFromServer(fd);
            Logger::log(LogLevel::INFO, "Closed client connection");
            clients.erase(fd);
        }
    }
}


void Server::stop() {
    if (serverFd >= 0) {
        clients.clear();

        Logger::log(LogLevel::DEBUG, "Stopping server on fd: " + std::to_string(serverFd));
        close(serverFd);
        serverFd = -1;
        Logger::log(LogLevel::INFO, "Server stopped");
    }
}
