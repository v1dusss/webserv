//
// Created by Emil Ebert on 01.05.25.
//

#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include "HttpRequest.h"
#include <memory>
#include <string>

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

public:
    HttpParser();

    bool parseRequestLine();

    bool parseHeaders();

    bool parseBody();

    HttpMethod stringToMethod(const std::string &method);


    bool parse(const char *data, size_t length);

    std::shared_ptr<HttpRequest> getRequest() const;

    void reset();

    bool isComplete() const { return state == ParseState::COMPLETE; }
    bool hasError() const { return state == ParseState::ERROR; }
};


#endif //HTTPPARSER_H
