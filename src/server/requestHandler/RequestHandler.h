//
// Created by Emil Ebert on 01.05.25.
//

#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <config/config.h>
#include <parser/http/HttpRequest.h>
#include <server/response/HttpResponse.h>
#include <optional>
#include <parser/cgi/CgiParser.h>

class ClientConnection;

enum class MultipartParseStateEnum {
    LOOKING_FOR_BOUNDARY,
    READING_HEADERS,
    READING_FILE_CONTENT
};

struct MultipartParseState {
    std::string boundary;
    std::string endBoundary;
    std::string parseBuffer;
    int fileWriteFd = -1;
    int uploadedFiles;
    std::vector<std::string> fileNames;
    MultipartParseStateEnum currentState;
};


class RequestHandler {
private:
    std::optional<RouteConfig> matchedRoute;
    const std::shared_ptr<HttpRequest> request;
    ClientConnection *client;
    ServerConfig &serverConfig;
    std::string routePath;
    std::string cgiPath;
    bool isFile = false;
    bool hasValidIndexFile = false;
    std::string indexFilePath;

    ssize_t bytesWrittenToCgi = 0;
    std::string cgiOutputBuffer{};
    // TODO: clean this code, by using an array of fileDescriptors that will be closed at the end
    int cgiOutputFd = -1;
    int cgiInputFd = -1;
    int cgiProcessId = -1;
    int fileWriteFd = -1;
    ssize_t postRequestCallbackId = -1;
    CgiParser cgiParser;

public:
    RequestHandler(ClientConnection *connection, const std::shared_ptr<HttpRequest> &request,
                   ServerConfig &serverConfig);

    ~RequestHandler();

    void execute();

    std::optional<HttpResponse> handleRequest();

    static HttpResponse handleCustomErrorPage(HttpResponse original, ServerConfig &serverConfig,
                                              std::optional<RouteConfig> matchedRoute);

    static std::string getMimeType(const std::string &path);

    static std::string getFileExtension(const std::string &path);

    static std::string urlDecode(const std::string &in);

    [[nodiscard]] std::optional<HttpResponse> handleMetrics() const;

private:
    void findRoute();

    void setRoutePath();

    void validateTargetPath();

    bool isCgiRequest();

    [[nodiscard]] std::string getFilePath() const;


    [[nodiscard]] HttpResponse handleGet() const;

    [[nodiscard]] std::optional<HttpResponse> handlePost();

    [[nodiscard]] std::optional<HttpResponse> handlePostMultipart(const std::string &contentType);

    void processMultipartBuffer(MultipartParseState *state);

    [[nodiscard]] std::optional<HttpResponse> handlePostTestFile();

    HttpResponse handlePut() const;

    [[nodiscard]] HttpResponse handleDelete() const;

    [[nodiscard]] std::optional<HttpResponse> handleCgi();

    bool writeRequestBodyToCgi(int pipe_fd, const std::string &body);

    [[nodiscard]] bool validateCgiEnvironment() const;

    void configureCgiChildProcess(int input_pipe[2], int output_pipe[2]) const;
};


#endif //REQUESTHANDLER_H
