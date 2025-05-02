//
// Created by Emil Ebert on 01.05.25.
//

#include "ClientConnection.h"

#include <unistd.h>

ClientConnection::ClientConnection(int clientFd, struct sockaddr_in clientAddr) {
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
    }
    return *this;
}
