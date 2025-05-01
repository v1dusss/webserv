//
// Created by Emil Ebert on 01.05.25.
//

#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <poll.h>
#include <netinet/in.h>
#include <parser/http/HttpParser.h>

class ClientConnection {
public:
    pollfd clientFd{};
    struct sockaddr_in clientAddr{};
    HttpParser parser{};

public:
    ClientConnection() = default;
    ClientConnection(int clientFd, struct sockaddr_in clientAddr);
    ClientConnection(const ClientConnection &other);
    ClientConnection &operator=(const ClientConnection &other);
};



#endif //CLIENTCONNECTION_H
