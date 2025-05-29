//
// Created by Emil Ebert on 13.05.25.
//

#ifndef CGIPARSER_H
#define CGIPARSER_H


#include <string>
#include <map>
#include <optional>
#include <memory>
#include <cstdio>
#include <server/buffer/SmartBuffer.h>

enum class CgiParseState {
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
};

class CgiParser {
public:
    struct CgiResult {
        std::map<std::string, std::string> headers;
        std::shared_ptr<SmartBuffer> body;
    };

private:
    CgiParseState state;
    std::string buffer;
    CgiResult result;
    ssize_t contentLength;

    bool parseHeaders();

    bool appendToBody(std::string data);

public:
    CgiParser();

    bool parse(const char *data, size_t length);

    bool isComplete() const { return state == CgiParseState::COMPLETE; }
    bool hasError() const { return state == CgiParseState::ERROR; }
    const CgiResult &getResult() const { return result; }
};


#endif //CGIPARSER_H
