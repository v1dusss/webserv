//
// Created by Emil Ebert on 01.05.25.
//

#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include "HttpRequest.h"
#include <memory>
#include <string>
#include <optional>
#include <ctime>
#include <server/response/HttpResponse.h>
#include "config/Config.h"

enum class ParseState {
    REQUEST_LINE,
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
};

class ClientConnection; // Forward declaration to avoid circular dependency

class HttpParser {
private:
    static ssize_t tmpFileCount;
    ParseState state;
    std::shared_ptr<HttpRequest> request;
    std::string buffer;
    size_t contentLength;
    bool chunkedTransfer;
    ClientConnection *clientConnection;
    HttpResponse::StatusCode errorCode = HttpResponse::StatusCode::BAD_REQUEST;


    unsigned long chunkSize = 0;
    bool hasChunkSize = false;

    bool parseChunkedBody();

public:
    std::time_t headerStart = 0;
    std::time_t bodyStart = 0;

    HttpParser(ClientConnection *clientConnection);

    ~HttpParser();

    bool parseRequestLine();

    bool parseHeaders();

    bool parseBody();

    bool appendToBody(const std::string &data);

    static std::optional<HttpMethod> stringToMethod(const std::string &method);


    bool parse(const char *data, size_t length);

    [[nodiscard]] std::shared_ptr<HttpRequest> getRequest() const;

    void reset();

    bool isComplete() const { return state == ParseState::COMPLETE; }
    bool hasError() const { return state == ParseState::ERROR; }

    const std::string &getBuffer() const { return buffer; }

    static bool isHttpStatusCode(int statusCode);

    [[nodiscard]] ParseState getState() const { return state; }

    [[nodiscard]] HttpResponse::StatusCode getErrorCode() const { return errorCode; }
};


#endif //HTTPPARSER_H
