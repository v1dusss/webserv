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

class ClientConnection {
public:
    int fd{};
    sockaddr_in clientAddr{};
    HttpParser parser{};
    size_t requestCount = 0;
    std::time_t lastPackageSend = 0;
    bool keepAlive = false;
    bool shouldClose = false;
    ServerConfig &config;
    std::string debugBuffer;

    std::optional<std::unique_ptr<RequestHandler>> requestHandler = std::nullopt;

private:
    std::optional<HttpResponse> response = std::nullopt;

public:
    ClientConnection() = delete;

    ClientConnection(int clientFd, struct sockaddr_in clientAddr, ServerConfig &config);

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
};


#endif //CLIENTCONNECTION_H
