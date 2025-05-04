//
// Created by Emil Ebert on 01.05.25.
//

#include "RequestHandler.h"

#include <unistd.h>
#include <sys/stat.h>
#include <filesystem>
#include <fcntl.h>
#include <algorithm>
#include <string>


#include <sys/types.h>
#include <arpa/inet.h>

#include "common/Logger.h"

RequestHandler::RequestHandler(ClientConnection &connection, const HttpRequest &request,
                               ServerConfig &serverConfig): request(request), connection(connection),
                                                            serverConfig(serverConfig) {
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(connection.clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    Logger::log(LogLevel::DEBUG, "IP: " + std::string(ipStr) +
                                 " Port: " + std::to_string(ntohs(connection.clientAddr.sin_port)) + " request: " +
                                 request.getMethodString() + " at " + request.uri);

    findRoute();
    setRoutePath();
}

void RequestHandler::findRoute() {
    size_t longestMatch = 0;
    for (const RouteConfig &route: serverConfig.routes) {
        if (request.getPath().find(route.location) == 0) {
            Logger::log(LogLevel::DEBUG, "Found route: " + route.location);
            if (std::find(route.allowedMethods.begin(), route.allowedMethods.end(), request.method) != route.
                allowedMethods.end() && route.location.length() > longestMatch) {
                matchedRoute = route;
                longestMatch = route.location.length();
            }
        }
    }
    if (longestMatch > 0) {
        Logger::log(LogLevel::DEBUG, "Matched route: " + matchedRoute->location);
        return;
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

    std::string uriSuffix = request.getPath().substr(route.location.length());
    if (!uriSuffix.empty() && uriSuffix[0] == '/') {
        uriSuffix.erase(0, 1);
    }

    if (!basePath.empty() && basePath.back() != '/')
        basePath += '/';
    routePath = basePath + uriSuffix;
    Logger::log(LogLevel::DEBUG, "set path to " + routePath);
}

void RequestHandler::validateTargetPath() {
    isFile = std::filesystem::is_regular_file(routePath);
    if (isFile)
        return;

    RouteConfig route = matchedRoute.value();
    if (route.index.empty() && serverConfig.index.empty())
        return;

    std::string indexFilePath = std::filesystem::path(routePath) / (!route.index.empty()
                                                                        ? route.index
                                                                        : serverConfig.index);

    hasValidIndexFile = std::filesystem::exists(indexFilePath) && std::filesystem::is_regular_file(indexFilePath) &&
                        access(indexFilePath.c_str(), R_OK) == 0;

    this->indexFilePath = indexFilePath;
}

bool RequestHandler::isCgiRequest() {
    if (matchedRoute->cgi_params.empty())
        return false;

    const std::string filePath = getFilePath();

    const std::string fileExtension = getFileExtension(filePath);
    if (fileExtension.empty())
        return false;

    if (matchedRoute->cgi_params.count(fileExtension) == 0)
        return false;

    const std::string cgiPath = matchedRoute->cgi_params.at(fileExtension);
    if (cgiPath.empty())
        return false;

    this->cgiPath = cgiPath;
    return true;
}

HttpResponse RequestHandler::handleRequest() {
    if (!matchedRoute.has_value()) {
        HttpResponse response(HttpResponse::StatusCode::NOT_FOUND);
        response.setBody("404 Not Found");
        return response;
    }

    validateTargetPath();

    if (isCgiRequest()) {
        Logger::log(LogLevel::DEBUG, "request is a CGI request");
        std::optional<HttpResponse> cgiResponse = handleCgi();

        if (cgiResponse.has_value())
            return cgiResponse.value();
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
            return Response::customResponse(HttpResponse::StatusCode::METHOD_NOT_ALLOWED, "405 Method Not Allowed");
    }
}

std::string RequestHandler::getFilePath() const {
    if (hasValidIndexFile)
        return this->indexFilePath;
    return this->routePath;
}
