//
// Created by Emil Ebert on 01.05.25.
//

#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <netinet/in.h>
#include <parser/http/HttpParser.h>

class ClientConnection {
public:
    int fd{};
    struct sockaddr_in clientAddr{};
    HttpParser parser{};
    int requests{};
    std::time_t start = std::time(nullptr);
    bool keepAlive = false;
    bool shouldClose = false;

    // used for debugging
    std::string buffer = "";

private:
    std::string rawResponse;
    bool hasResponse = false;

public:
    ClientConnection() = default;

    ClientConnection(int clientFd, struct sockaddr_in clientAddr);

    ClientConnection(const ClientConnection &other);

    ClientConnection &operator=(const ClientConnection &other);

    void setResponse(const std::string &response) {
        rawResponse = response;
        hasResponse = true;
    }

    void clearResponse() {
        rawResponse.clear();
        hasResponse = false;
    }

    bool hasPendingResponse() const {
        return hasResponse;
    }

    std::string getResponse() {
        return rawResponse;
    }
};


#endif //CLIENTCONNECTION_H
