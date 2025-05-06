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

#include "response/HttpResponse.h"

class ClientConnection {
public:
    int fd{};
    sockaddr_in clientAddr{};
    HttpParser parser{};
    size_t requestCount = 0;
    std::time_t lastPackageSend = 0;
    bool keepAlive = false;
    bool shouldClose = false;

    // used for debugging
    std::string buffer = "";

private:
    std::optional<HttpResponse> response = std::nullopt;

public:
    ClientConnection() = default;

    ClientConnection(int clientFd, struct sockaddr_in clientAddr);

    ~ClientConnection();

    ClientConnection(const ClientConnection &other);

    ClientConnection &operator=(const ClientConnection &other);

    void setResponse(const HttpResponse &response);

    void clearResponse() {
        if (response != std::nullopt && response.value().getBodyFd()) {
            close(response.value().getBodyFd());
        }
        response = std::nullopt;
    }

    [[nodiscard]] bool hasPendingResponse() const {
        return response != std::nullopt;
    }

    std::optional<HttpResponse> &getResponse() {
        return response;
    }
};


#endif //CLIENTCONNECTION_H
