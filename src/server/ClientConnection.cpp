//
// Created by Emil Ebert on 01.05.25.
//

#include "ClientConnection.h"

#include <unistd.h>

ClientConnection::ClientConnection(const int clientFd, const sockaddr_in clientAddr) {
    this->fd = clientFd;
    this->clientAddr = clientAddr;
}

ClientConnection::ClientConnection(const ClientConnection &other) {
    *this = other;
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
        this->rawResponse = other.rawResponse;
        this->hasResponse = other.hasResponse;
    }
    return *this;
}

void ClientConnection::setResponse(const std::string &response) {
    rawResponse = response;
    hasResponse = true;

    buffer.clear();
    parser.reset();
    requestCount++;
    if (!keepAlive)
        shouldClose = true;
}
