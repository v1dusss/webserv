//
// Created by Emil Ebert on 01.05.25.
//

#include "ClientConnection.h"

#include <unistd.h>
#include <webserv.h>
#include <common/Logger.h>
#include <vector>

#include "FdHandler.h"
#include "requestHandler/RequestHandler.h"

ClientConnection::ClientConnection(const int clientFd, const sockaddr_in clientAddr, ServerConfig &config): config(
    config) {
    this->fd = clientFd;
    this->clientAddr = clientAddr;
    parser.setClientLimits(config.client_max_body_size, config.client_max_header_size);
    FdHandler::addFd(clientFd, POLLIN | POLLOUT, [this](const int fd, short events) {
        (void) fd;
        if (events & POLLIN && !requestHandler.has_value())
            this->handleInput();
        if (events & POLLOUT)
            this->handleOutput();
        return shouldClose;
    });
}

ClientConnection::~ClientConnection() {
    FdHandler::removeFd(this->fd);
    if (requestHandler.has_value())
        requestHandler.value().reset();
    this->clearResponse();
    if (fd != -1) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        Logger::log(LogLevel::DEBUG, "closed client fd: " + std::to_string(fd));
        fd = -1;
    }
}

void ClientConnection::handleInput() {
    char buffer[1024];
    const ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        Logger::log(LogLevel::ERROR, "Failed to read from client");
        return;
    }

    if (bytesRead == 0) {
        shouldClose = true;
        return;
    }
    this->buffer += std::string(buffer, bytesRead);
    //std::cout << clientConnection.buffer << std::endl;
    //  std::cout << "-------------------------" << std::endl;

    if (parser.parse(buffer, bytesRead)) {
        const auto request = parser.getRequest();
        request->body = "test";

        keepAlive = request->getHeader("Connection") == "keep-alive";

        Logger::log(LogLevel::INFO, "Request Parsed");
        //  request->printRequest();

        requestHandler = std::make_optional(std::make_unique<RequestHandler>(this, *request, config));
        requestHandler.value()->execute();
        return;
    }

    if (parser.hasError()) {
        std::cout << buffer << std::endl;

        const HttpResponse response = HttpResponse::html(HttpResponse::StatusCode::BAD_REQUEST);
        setResponse(RequestHandler::handleCustomErrorPage(response, config, std::nullopt));
    }
}

void ClientConnection::handleOutput() {
    if (hasPendingResponse()) {
        HttpResponse &response = getResponse().value();
        if (keepAlive)
            response.setHeader("Connection", "keep-alive");
        else
            response.setHeader("Connection", "close");

        if (response.isChunkedEncoding()) {
            handleFileOutput();
            return;
        }
        const std::string responseBuffer = response.toString();
        const size_t bytesWritten = write(fd, responseBuffer.c_str(), responseBuffer.size());

        if (bytesWritten < responseBuffer.size()) {
            Logger::log(LogLevel::ERROR, "Failed to write full response to client");
            return;
        }

        lastPackageSend = std::time(nullptr);
        Logger::log(LogLevel::INFO, "Client response sent");
        clearResponse();
    }
}

void ClientConnection::handleFileOutput() {
    if (!response.value().alreadySendHeader) {
        const std::string header = response.value().toHeaderString();
        if (write(fd, header.c_str(), header.length()) < 0) {
            Logger::log(LogLevel::ERROR, "Failed to write header to client");
            clearResponse();
            return;
        }
        response.value().alreadySendHeader = true;
    }

    const int bodyFd = response.value().getBodyFd();
    std::vector<char> buffer(config.send_body_buffer_size);

    pollfd pollfd{};
    pollfd.fd = bodyFd;
    pollfd.events = POLLIN;

    const int pollResult = poll(&pollfd, 1, READ_FILE_TIMEOUT);
    if (pollResult < 0) {
        Logger::log(LogLevel::ERROR, "Poll error while reading from client");
        clearResponse();
        return;
    }

    if (pollResult == 0) {
        Logger::log(LogLevel::ERROR, "Timeout while reading from client");
        clearResponse();
        return;
    }

    const ssize_t bytesRead = read(bodyFd, buffer.data(), buffer.size());

    if (bytesRead < 0) {
        Logger::log(LogLevel::ERROR, "Failed to read from file descriptor");
        clearResponse();
        return;
    }

    if (bytesRead == 0) {
        if (write(fd, "0\r\n\r\n", 5) < 0)
            Logger::log(LogLevel::ERROR, "Failed to write final chunk to client");
        lastPackageSend = std::time(nullptr);
        Logger::log(LogLevel::INFO, "Client response sent");
        clearResponse();
        return;
    }

    std::stringstream chunkHeader;
    chunkHeader << std::hex << bytesRead << "\r\n";
    const std::string header = chunkHeader.str();

    // Write the chunk header
    if (write(fd, header.c_str(), header.length()) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to write chunk header to client");
        clearResponse();
    }

    // Write the chunk data
    if (write(fd, buffer.data(), bytesRead) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to write chunk data to client");
        clearResponse();
    }

    // Write the trailing CRLF
    if (write(fd, "\r\n", 2) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to write chunk trailing CRLF to client");
        clearResponse();
    }
}


void ClientConnection::clearResponse() {
    if (response != std::nullopt && response.value().getBodyFd()) {
        close(response.value().getBodyFd());
    }
    response = std::nullopt;
    if (!keepAlive)
        shouldClose = true;

    requestHandler.reset();
}

void ClientConnection::setResponse(const HttpResponse &response) {
    this->response = response;
    buffer.clear();
    parser.reset();
    requestCount++;
}
