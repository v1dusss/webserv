//
// Created by Emil Ebert on 01.05.25.
//

#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include "HttpRequest.h"
#include <memory>
#include <string>
#include <optional>

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
    int client_max_body_size;
    int client_header_buffer_size;

public:
    HttpParser(int client_max_body_size, int client_header_buffer_size);

    bool parseRequestLine();

    bool parseHeaders();

    bool parseBody();

    static std::optional<HttpMethod> stringToMethod(const std::string &method);


    bool parse(const char *data, size_t length);

    std::shared_ptr<HttpRequest> getRequest() const;

    void reset();

    bool isComplete() const { return state == ParseState::COMPLETE; }
    bool hasError() const { return state == ParseState::ERROR; }

    const std::string &getBuffer() const { return buffer; }
};


#endif //HTTPPARSER_H
