//
// Created by Emil Ebert on 01.05.25.
//

#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <config/config.h>
#include <parser/http/HttpRequest.h>
#include <server/ClientConnection.h>
#include <server/response/HttpResponse.h>

class RequestHandler {
  private:
    std::optional<RouteConfig> matchedRoute;
    const HttpRequest request;
    ClientConnection& connection;
    ServerConfig& serverConfig;
    std::string routePath;

public:
    RequestHandler(ClientConnection& connection, const HttpRequest& request,
                   ServerConfig& serverConfig);
    HttpResponse handleRequest();

private:
    void findRoute();
    void setRoutePath();

    HttpResponse handleGet();
    HttpResponse handlePost();
    HttpResponse handlePut();
    HttpResponse handleDelete();

};


#endif //REQUESTHANDLER_H
