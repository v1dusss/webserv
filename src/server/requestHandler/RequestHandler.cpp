//
// Created by Emil Ebert on 01.05.25.
//

#include "RequestHandler.h"

#include <unistd.h>
#include <sys/stat.h>
#include <filesystem>
#include <fcntl.h>
#include <algorithm>
#include <fstream>
#include <regex>
#include <csignal>
#include <string>


#include <sys/types.h>
#include <arpa/inet.h>
#include <server/handler/CallbackHandler.h>
#include <server/FdHandler.h>
#include <sys/poll.h>

#include "common/Logger.h"
#include "webserv.h"
#include "server/ClientConnection.h"


RequestHandler::RequestHandler(ClientConnection *connection, const std::shared_ptr<HttpRequest> &request,

                               ServerConfig &serverConfig): request(request), client(connection),
                                                            serverConfig(serverConfig) {
    Logger::log(LogLevel::DEBUG,
                                 " Port: " + std::to_string(ntohs(connection->clientAddr.sin_port)) + " request: " +
                                 request->getMethodString() + " at " + request->uri);

    findRoute();
    setRoutePath();
}

RequestHandler::~RequestHandler() {
    if (cgiInputFd != -1) {
        FdHandler::removeFd(cgiInputFd);
        close(cgiInputFd);
        cgiInputFd = -1;
    }
    if (cgiOutputFd != -1) {
        FdHandler::removeFd(cgiOutputFd);
        close(cgiOutputFd);
        cgiOutputFd = -1;
    }
    if (fileWriteFd != -1) {
        FdHandler::removeFd(fileWriteFd);
        close(fileWriteFd);
        fileWriteFd = -1;
    }
    if (cgiProcessId != -1) {
        kill(cgiProcessId, SIGINT);
    }
    if (postRequestCallbackId != -1) {
        CallbackHandler::unregisterCallback(postRequestCallbackId);
        postRequestCallbackId = -1;
    }
}


void RequestHandler::findRoute() {
    size_t longestMatch = 0;
    for (const RouteConfig &route: serverConfig.routes) {
        if (route.type == LocationType::EXACT && request->getPath() == route.location) {
            matchedRoute = route;
            longestMatch = route.location.length();
            break;
        }
        if (route.type == LocationType::REGEX &&
            std::regex_match(request->getPath(), std::regex(route.location))) {
            matchedRoute = route;
            longestMatch = route.location.length();
            break;
        }
        if (route.type == LocationType::PREFIX && request->getPath().find(route.location) == 0) {
            if (route.location.length() > longestMatch) {
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
    Logger::log(LogLevel::WARNING, "No matching route found for URI: " + request->uri);
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

    std::string uriSuffix = request->getPath().substr(route.location.length());
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

    isDirectory = std::filesystem::is_directory(routePath);
    if (!isDirectory) {
        Logger::log(LogLevel::ERROR, "Target path is neither a file nor a directory: " + routePath);
        return;
    }

    const RouteConfig route = matchedRoute.value();
    if (route.index.empty() && serverConfig.index.empty())
        return;

    const std::string indexFilePath = std::filesystem::path(routePath) / (!route.index.empty()
                                                                              ? route.index
                                                                              : serverConfig.index);
    hasValidIndexFile = std::filesystem::is_regular_file(indexFilePath);

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

void RequestHandler::execute() {
    const auto response = handleRequest();
    if (response.has_value()) {
        setResponse(response.value());
    }
}

std::optional<HttpResponse> RequestHandler::handleRequest() {
    if (!matchedRoute.has_value())
        return HttpResponse::html(HttpResponse::NOT_FOUND);

    if (std::find(matchedRoute->allowedMethods.begin(), matchedRoute->allowedMethods.end(), request->method) ==
        matchedRoute->allowedMethods.end()) {
        return HttpResponse::html(HttpResponse::StatusCode::METHOD_NOT_ALLOWED);
    }

    if (matchedRoute->internalHandler != nullptr) {
        Logger::log(LogLevel::DEBUG, "Handling internal request for URI: " + request->uri);
        return matchedRoute->internalHandler(request);
    }

    if (matchedRoute->deny_all) {
        Logger::log(LogLevel::WARNING, "Access denied for URI: " + request->uri);
        return HttpResponse::html(HttpResponse::StatusCode::FORBIDDEN);
    }

    if (matchedRoute->return_directive.first != -1) {
        HttpResponse response(matchedRoute->return_directive.first);
        response.setHeader("Location", matchedRoute->return_directive.second);
        response.setBody(matchedRoute->return_directive.second);
        return response;
    }

    try {
        validateTargetPath();
    } catch (std::exception &e) {
        Logger::log(LogLevel::ERROR, "Error validating target path: " + std::string(e.what()));
        return HttpResponse::html(HttpResponse::StatusCode::FORBIDDEN);
    }

    if (isCgiRequest()) {
        Logger::log(LogLevel::DEBUG, "request is a CGI request");
        if (!validateCgiEnvironment())
            return HttpResponse::html(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                      "CGI Error: Invalid CGI environment");
        return handleCgi();
    }


    switch (request->method) {
        case GET:
            return handleGet();
        case POST:
            return handlePost();
        case PUT:
            return handlePut();
        case DELETE:
            return handleDelete();
        default:
            return HttpResponse::html(HttpResponse::StatusCode::METHOD_NOT_ALLOWED);
    }
}

HttpResponse RequestHandler::handleCustomErrorPage(HttpResponse original, ServerConfig &serverConfig,
                                                   std::optional<RouteConfig> matchedRoute) {
    std::string errorPagePath;

    if (matchedRoute.has_value() && matchedRoute->error_pages.count(original.getStatus()))
        errorPagePath = matchedRoute->error_pages[original.getStatus()];
    else if (serverConfig.error_pages.count(original.getStatus()))
        errorPagePath = serverConfig.error_pages[original.getStatus()];
    else
        return original;


    if (access(errorPagePath.c_str(), R_OK) != 0 || !std::filesystem::is_regular_file(errorPagePath)) {
        Logger::log(LogLevel::ERROR, "error page has an invalid path: " + errorPagePath);
        return original;
    }
    const int fd = open(errorPagePath.c_str(), O_RDONLY);
    if (fd < 0) {
        Logger::log(LogLevel::ERROR, "error page does not exist: " + errorPagePath);
        return original;
    }

    HttpResponse newResponse(HttpResponse::StatusCode::OK);
    newResponse.enableChunkedEncoding(std::make_shared<SmartBuffer>(fd));
    newResponse.setHeader("Content-Type", getMimeType(errorPagePath));
    newResponse.setStatus(original.getStatus());
    return newResponse;
}


std::string RequestHandler::getFilePath() const {
    if (hasValidIndexFile)
        return this->indexFilePath;
    return this->routePath;
}

void RequestHandler::setResponse(const HttpResponse &response) const {
    this->client->setResponse(handleCustomErrorPage(response, serverConfig, matchedRoute));
}
