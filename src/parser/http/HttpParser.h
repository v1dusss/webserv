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

enum class ParseState {
    REQUEST_LINE,
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
};

class HttpParser {
private:
    ParseState state;
    std::shared_ptr<HttpRequest> request;
    std::string buffer;
    size_t contentLength;
    bool chunkedTransfer;
    size_t client_max_body_size;
    size_t client_max_header_size;
    size_t totalHeaderSize = 0;
    size_t totalBodySize = 0;

public:
    std::time_t headerStart = 0;
    std::time_t bodyStart = 0;

    HttpParser();

    void setClientLimits(const size_t maxBodySize, const size_t maxHeaderSize) {
        client_max_body_size = maxBodySize;
        client_max_header_size = maxHeaderSize;
    }


    bool parseRequestLine();

    bool parseHeaders();

    bool parseBody();

    static std::optional<HttpMethod> stringToMethod(const std::string &method);


    bool parse(const char *data, size_t length);

    [[nodiscard]] std::shared_ptr<HttpRequest> getRequest() const;

    void reset();

    bool isComplete() const { return state == ParseState::COMPLETE; }
    bool hasError() const { return state == ParseState::ERROR; }

    const std::string &getBuffer() const { return buffer; }

    static bool isHttpStatusCode(int statusCode);
};


#endif //HTTPPARSER_H
