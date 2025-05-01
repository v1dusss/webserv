//
// Created by Emil Ebert on 01.05.25.
//

#include "HttpParser.h"

#include "HttpParser.h"
#include "common/Logger.h"
#include <sstream>
#include <algorithm>

HttpParser::HttpParser()
    : state(ParseState::REQUEST_LINE),
      request(std::make_shared<HttpRequest>()),
      contentLength(0),
      chunkedTransfer(false) {
}

bool HttpParser::parse(const char *data, size_t length) {
    if (state == ParseState::COMPLETE || state == ParseState::ERROR)
        return false;

    buffer.append(data, length);

    bool needMoreData = false;

    while (!needMoreData) {
        switch (state) {
            case ParseState::REQUEST_LINE:
                needMoreData = !parseRequestLine();
                break;

            case ParseState::HEADERS:
                needMoreData = !parseHeaders();
                break;

            case ParseState::BODY:
                needMoreData = !parseBody();
                break;

            case ParseState::COMPLETE:
                return true;

            case ParseState::ERROR:
                return false;
        }
    }

    return state == ParseState::COMPLETE;
}

bool HttpParser::parseRequestLine() {
    size_t endPos = buffer.find("\r\n");
    if (endPos == std::string::npos)
        return false;

    std::string line = buffer.substr(0, endPos);
    buffer.erase(0, endPos + 2);

    std::istringstream iss(line);
    std::string methodStr, uri, version;

    if (!(iss >> methodStr >> uri >> version)) {
        Logger::log(LogLevel::ERROR, "Invalid HTTP request line");
        state = ParseState::ERROR;
        return false;
    }

    request->method = stringToMethod(methodStr);
    request->uri = uri;
    request->version = version;

    state = ParseState::HEADERS;
    return true;
}

bool HttpParser::parseHeaders() {
    while (true) {
        size_t endPos = buffer.find("\r\n");
        if (endPos == std::string::npos)
            return false;

        if (endPos == 0) {
            buffer.erase(0, 2);

            std::string contentLengthStr = request->getHeader("Content-Length");
            if (!contentLengthStr.empty()) {
                try {
                    contentLength = std::stoi(contentLengthStr);
                } catch (...) {
                    state = ParseState::ERROR;
                    return false;
                }
            }

            std::string transferEncoding = request->getHeader("Transfer-Encoding");
            chunkedTransfer = (transferEncoding == "chunked");

            if (contentLength > 0 || chunkedTransfer) {
                state = ParseState::BODY;
            } else {
                state = ParseState::COMPLETE;
            }
            return true;
        }

        std::string line = buffer.substr(0, endPos);
        buffer.erase(0, endPos + 2);

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            state = ParseState::ERROR;
            return false;
        }

        std::string name = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        value.erase(0, value.find_first_not_of(" \t"));

        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        request->headers[name] = value;
    }
}

bool HttpParser::parseBody() {
    if (chunkedTransfer) {
        Logger::log(LogLevel::ERROR, "Chunked encoding not yet implemented");
        state = ParseState::ERROR;
        return false;
    }

    if (buffer.length() < contentLength)
        return false;

    request->body = buffer.substr(0, contentLength);
    buffer.erase(0, contentLength);
    state = ParseState::COMPLETE;
    return true;
}

HttpMethod HttpParser::stringToMethod(const std::string &method) {
    if (method == "GET") return GET;
    if (method == "POST") return POST;
    if (method == "PUT") return PUT;
    if (method == "DELETE") return DELETE;
    if (method == "PATCH") return PATCH;
    if (method == "OPTIONS") return OPTIONS;

    return GET;
}

std::shared_ptr<HttpRequest> HttpParser::getRequest() const {
    return request;
}

void HttpParser::reset() {
    state = ParseState::REQUEST_LINE;
    request = std::make_shared<HttpRequest>();
    buffer.clear();
    contentLength = 0;
    chunkedTransfer = false;
}
