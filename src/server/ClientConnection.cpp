//
// Created by Emil Ebert on 01.05.25.
//

#include "ClientConnection.h"

#include <unistd.h>
#include <webserv.h>
#include <common/Logger.h>

#include "FdHandler.h"
#include "requestHandler/RequestHandler.h"
#include <execinfo.h>
#include <iostream>
#include <cstdlib>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


ClientConnection::ClientConnection(const int clientFd,
                                   const sockaddr_in clientAddr,
                                   const Server *connectedServer): fd(clientFd),
                                                                    clientAddr(clientAddr), parser(this),
                                                                    connectedServer(connectedServer) {
    // TODO: change to http config part
    parser.setClientLimits(100000, 1000, 100000);
    config.client_body_timeout = 0;
    config.keepalive_timeout = 0;
    config.keepalive_requests = 0;
    config.headerConfig.client_header_timeout = 2;
    config.headerConfig.client_max_header_size = 8192;
    config.headerConfig.client_max_header_count = 100;

    FdHandler::addFd(clientFd, POLLIN | POLLOUT, [this](const int fd, const short events) {
        (void) fd;
        if (shouldClose)
            return true;
        if (events & POLLIN)
            this->handleInput();
        if (events & POLLOUT)
            this->handleOutput();
        return false;
    });
}

ClientConnection::~ClientConnection() {
    FdHandler::removeFd(this->fd);
    this->clearResponse();
    parser.reset();
    if (fd != -1) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        Logger::log(LogLevel::DEBUG, "closed client fd: " + std::to_string(fd));
        fd = -1;
    }
    delete requestHandler;
    requestHandler = nullptr;
}

void ClientConnection::handleInput() {
    // TODO: fix magic number
    char buffer[40001];
    const ssize_t bytesRead = read(fd, buffer, 40000);
    if (bytesRead < 0) {
        Logger::log(LogLevel::ERROR, "Failed to read from client fd: " + std::to_string(fd));
        Logger::log(LogLevel::ERROR, strerror(errno));

        Logger::log(LogLevel::ERROR, "should close client fd: " + std::to_string(shouldClose));

        return;
    }

    if (bytesRead == 0) {
        shouldClose = true;
        return;
    }

    buffer[bytesRead] = '\0';


    //std::cout << "Received data from client fd: " << fd << " size: " << bytesRead << std::endl;
    //std::cout << buffer << std::endl;

    // std::cout << "-------------------------" << std::endl;

    if (parser.parse(buffer, bytesRead)) {
        const auto request = parser.getRequest();

        keepAlive = request->getHeader("Connection") == "keep-alive";

        Logger::log(LogLevel::INFO, "Request Parsed");


        delete requestHandler;
        requestHandler = new RequestHandler(this, request, config);
        requestHandler->execute();
        parser.reset();
        debugBuffer.clear();
        return;
    }

    if (parser.hasError()) {
        const HttpResponse response = HttpResponse::html(HttpResponse::StatusCode::BAD_REQUEST);
        setResponse(RequestHandler::handleCustomErrorPage(response, config, std::nullopt));
        parser.reset();
        debugBuffer.clear();
    }
}

void ClientConnection::handleOutput() {
    if (hasPendingResponse()) {
        HttpResponse &response = getResponse().value();
        if (response.getBody()->isStillWriting())
            return;
        if (keepAlive)
            response.setHeader("Connection", "keep-alive");
        else
            response.setHeader("Connection", "close");

        handleFileOutput();
    }
}

void ClientConnection::handleFileOutput() {
    if (!response.value().alreadySendHeader) {
        const std::string header = response.value().toHeaderString();
        std::cout << header << std::endl;
        if (send(fd, header.c_str(), header.length(), MSG_NOSIGNAL) < 0) {
            Logger::log(LogLevel::ERROR, "Failed to write header to client");
            clearResponse();
            return;
        }
        response.value().alreadySendHeader = true;
        return;
    }


    std::shared_ptr<SmartBuffer> body = response->getBody();
    body->read(1024);

    const std::string readBuffer = body->getReadBuffer();

    if (!readBuffer.empty()) {
        std::stringstream chunkHeader;
        chunkHeader << std::hex << readBuffer.length() << "\r\n";
        const std::string chunkHeaderStr = chunkHeader.str();

        // Write the chunk header
        if (send(fd, chunkHeaderStr.c_str(), chunkHeaderStr.length(), MSG_NOSIGNAL) < 0) {
            Logger::log(LogLevel::ERROR, "Failed to write chunk header to client");
            clearResponse();
            return;
        }


        ssize_t bytesWritten = send(fd, readBuffer.c_str(), readBuffer.length(), MSG_NOSIGNAL);
        // Write the chunk data
        if (bytesWritten < 0) {
            Logger::log(LogLevel::ERROR, "Failed to write chunk data to client");
            clearResponse();
            return;
        }

        body->cleanReadBuffer(bytesWritten);

        // Write the trailing CRLF
        if (send(fd, "\r\n", 2, MSG_NOSIGNAL) < 0) {
            Logger::log(LogLevel::ERROR, "Failed to write chunk trailing CRLF to client");
            clearResponse();
        }
    }


    if (body->getReadPos() >= body->getSize()) {
        if (send(fd, "0\r\n\r\n", 5, MSG_NOSIGNAL) < 0)
            Logger::log(LogLevel::ERROR, "Failed to write final chunk to client");
        lastPackageSend = std::time(nullptr);
        Logger::log(LogLevel::INFO, "Client response sent");
        clearResponse();
    }
}


void ClientConnection::clearResponse() {
    response.reset();
    if (!keepAlive) {
        shouldClose = true;
    }

    delete requestHandler;
    requestHandler = nullptr;
}

void ClientConnection::setResponse(const HttpResponse &response) {
    this->response = response;
    parser.reset();
    requestCount++;
}
