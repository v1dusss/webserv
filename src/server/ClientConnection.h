//
// Created by Emil Ebert on 01.05.25.
//

#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <unistd.h>
#include <netinet/in.h>
#include <parser/http/HttpParser.h>
#include <optional>
#include <string>
#include <ctime>

#include "requestHandler/RequestHandler.h"
#include "response/HttpResponse.h"
#include <iostream>

class Server;

class ClientConnection {
public:
    int fd = -1;
    sockaddr_in clientAddr{};
    HttpParser parser;
    size_t requestCount = 0;
    std::time_t lastPackageSend = 0;
    bool keepAlive = false;
    bool shouldClose = false;
    const Server *connectedServer;
    ServerConfig config;

private:
    std::optional<HttpResponse> response = std::nullopt;
    RequestHandler *requestHandler = nullptr;
    std::string debugBuffer;

public:
    ClientConnection() = delete;

    ClientConnection(int clientFd, struct sockaddr_in clientAddr, const Server *connectedServer);

    ~ClientConnection();

    void handleInput();

    void handleOutput();

    void handleFileOutput();

    void setResponse(const HttpResponse &response);

    void clearResponse();

    [[nodiscard]] bool hasPendingResponse() const {
        return response.has_value();
    }

    std::optional<HttpResponse> &getResponse() {
        return response;
    }

    void setConfig(const ServerConfig &config);
};


#endif //CLIENTCONNECTION_H
