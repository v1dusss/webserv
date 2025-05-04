//
// Created by Emil Ebert on 01.05.25.
//

#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <config/config.h>
#include <parser/http/HttpRequest.h>
#include <server/ClientConnection.h>
#include <server/response/HttpResponse.h>

namespace Response {
    HttpResponse notFoundResponse();

    HttpResponse customResponse(int statusCode, const std::string &body,
                                const std::string &contentType = "text/plain");
}

class RequestHandler {
private:
    std::optional<RouteConfig> matchedRoute;
    const HttpRequest request;
    ClientConnection &connection;
    ServerConfig &serverConfig;
    std::string routePath;
    std::string cgiPath;
    bool isFile;
    bool hasValidIndexFile = false;
    std::string indexFilePath;

public:
    RequestHandler(ClientConnection &connection, const HttpRequest &request,
                   ServerConfig &serverConfig);

    HttpResponse handleRequest();

    static std::string getMimeType(const std::string &path);

    static std::string getFileExtension(const std::string &path);

private:
    void findRoute();

    void setRoutePath();

    void validateTargetPath();

    bool isCgiRequest();

    [[nodiscard]] std::string getFilePath() const;


    HttpResponse handleGet();

    HttpResponse handlePost() const;

    HttpResponse handlePut();

    HttpResponse handleDelete();

    std::optional<HttpResponse> handleCgi() const;
    [[nodiscard]] bool validateCgiEnvironment() const;
    void configureCgiChildProcess(int input_pipe[2], int output_pipe[2]) const;

};


#endif //REQUESTHANDLER_H
