//
// Created by Emil Ebert on 01.05.25.
//

#include "ClientConnection.h"

#include <unistd.h>
#include <common/Logger.h>

ClientConnection::ClientConnection(const int clientFd, const sockaddr_in clientAddr) {
    this->fd = clientFd;
    this->clientAddr = clientAddr;
}

ClientConnection::ClientConnection(const ClientConnection &other) {
    *this = other;
}

ClientConnection::~ClientConnection() {
    this->clearResponse();
    if (fd != -1) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        Logger::log(LogLevel::DEBUG, "closed client fd: " + std::to_string(fd));
        fd = -1;
    }
}

ClientConnection &ClientConnection::operator=(const ClientConnection &other) {
    if (this != &other) {
        this->fd = other.fd;
        this->clientAddr = other.clientAddr;
        this->parser = other.parser;
        this->requestCount = other.requestCount;
        this->lastPackageSend = other.lastPackageSend;
        this->keepAlive = other.keepAlive;
        this->shouldClose = other.shouldClose;
        this->buffer = other.buffer;
        this->response = other.response;
    }
    return *this;
}

void ClientConnection::setResponse(const HttpResponse &response) {
    this->response = response;
    buffer.clear();
    parser.reset();
    requestCount++;
    if (!keepAlive)
        shouldClose = true;
}
