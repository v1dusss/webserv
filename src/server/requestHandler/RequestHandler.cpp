//
// Created by Emil Ebert on 01.05.25.
//

#include "RequestHandler.h"
#include "common/Logger.h"

RequestHandler::RequestHandler(ClientConnection &connection, const HttpRequest &request,
                               ServerConfig &serverConfig): request(request), connection(connection),
                                                            serverConfig(serverConfig) {
    findRoute();
    setRoutePath();
}

void RequestHandler::findRoute() {
    size_t longestMatch = 0;
    for (const RouteConfig &route: serverConfig.routes) {
        if (request.uri.find(route.path) == 0) {
            if (std::find(route.allowedMethods.begin(), route.allowedMethods.end(), request.method) != route.
                allowedMethods.end() && route.path.length() > longestMatch) {
                matchedRoute = route;
                longestMatch = route.path.length();
                Logger::log(LogLevel::DEBUG, "Matched route: " + route.path);
                return;
            }
        }
    }
    matchedRoute.reset();
    Logger::log(LogLevel::WARNING, "No matching route found for URI: " + request.uri);
}

void RequestHandler::setRoutePath() {
    if (!matchedRoute.has_value()) {
        routePath.clear();
        return;
    }

    const RouteConfig &route = matchedRoute.value();
    std::string basePath;

    if (!route.alias.empty()) {
        basePath = route.alias;
    } else if (!route.root.empty()) {
        basePath = route.root;
    } else {
        basePath = serverConfig.root;
    }

    std::string uriSuffix = request.uri.substr(route.path.length());
    if (!uriSuffix.empty() && uriSuffix[0] == '/') {
        uriSuffix.erase(0, 1);
    }

    if (!basePath.empty() && basePath.back() != '/')
        basePath += '/';
    routePath = basePath + uriSuffix;
    Logger::log(LogLevel::DEBUG, "set path to " + routePath);
}

HttpResponse RequestHandler::handleRequest() {
    if (!matchedRoute.has_value()) {
        HttpResponse response(HttpResponse::StatusCode::NOT_FOUND);
        response.setBody("404 Not Found");
        return response;
    }

    switch (request.method) {
        case GET:
            return handleGet();
        case POST:
            return handlePost();
        case PUT:
            return handlePut();
        case DELETE:
            return handleDelete();
        default:
            HttpResponse response(HttpResponse::StatusCode::METHOD_NOT_ALLOWED);
            response.setBody("405 Method Not Allowed");
            return response;
    }
}