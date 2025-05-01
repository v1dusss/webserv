//
// Created by Emil Ebert on 01.05.25.
//

#include "ClientConnection.h"

ClientConnection::ClientConnection(int clientFd, struct sockaddr_in clientAddr) {
    this->clientFd.fd = clientFd;
    this->clientFd.events = POLLIN;
    this->clientAddr = clientAddr;
}

ClientConnection::ClientConnection(const ClientConnection &other) {
    this->clientFd = other.clientFd;
    this->clientAddr = other.clientAddr;
}

ClientConnection &ClientConnection::operator=(const ClientConnection &other) {
    if (this != &other) {
        this->clientFd = other.clientFd;
        this->clientAddr = other.clientAddr;
    }
    return *this;
}