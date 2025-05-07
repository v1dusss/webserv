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

ClientConnection::~ClientConnection() {
    this->clearResponse();
    if (fd != -1) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        Logger::log(LogLevel::DEBUG, "closed client fd: " + std::to_string(fd));
        fd = -1;
    }
}

void ClientConnection::setResponse(const HttpResponse &response) {
    this->response = response;
    buffer.clear();
    parser.reset();
    requestCount++;
}
