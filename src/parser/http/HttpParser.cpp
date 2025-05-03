//
// Created by Emil Ebert on 01.05.25.
//

#include "HttpParser.h"

#include "HttpParser.h"
#include "common/Logger.h"
#include <sstream>
#include <algorithm>
#include <regex>

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

    return false;
}

bool HttpParser::parseRequestLine() {
    size_t endPos = buffer.find("\r\n");
    if (endPos == std::string::npos)
        return false;

    constexpr size_t MAX_REQUEST_LINE_LENGTH = 8192;
    if (endPos > MAX_REQUEST_LINE_LENGTH) {
        Logger::log(LogLevel::ERROR, "Request line too long");
        state = ParseState::ERROR;
        return false;
    }

    std::string line = buffer.substr(0, endPos);
    buffer.erase(0, endPos + 2);

    std::istringstream iss(line);
    std::string methodStr, uri, version;

    if (!(iss >> methodStr >> uri >> version)) {
        Logger::log(LogLevel::ERROR, "Invalid HTTP request line format");
        state = ParseState::ERROR;
        return false;
    }

    auto method = stringToMethod(methodStr);
    if (method == std::nullopt) {
        Logger::log(LogLevel::ERROR, "Invalid HTTP method: " + methodStr);
        state = ParseState::ERROR;
        return false;
    }

    if ((uri.empty() || uri[0] != '/') && uri.find("://") == std::string::npos) {
        Logger::log(LogLevel::ERROR, "Invalid URI format: " + uri);
        state = ParseState::ERROR;
        return false;
    }

    if (version.length() < 8 || version.substr(0, 5) != "HTTP/" ||
        version[5] != '1' || version[6] != '.' ||
        (version[7] != '0' && version[7] != '1')) {
        Logger::log(LogLevel::ERROR, "Invalid HTTP version: " + version);
        state = ParseState::ERROR;
        return false;
    }

    request->method = method.value();
    request->uri = uri;
    request->version = version;

    state = ParseState::HEADERS;
    return true;
}

bool HttpParser::parseHeaders() {
    constexpr size_t MAX_HEADERS = 100;
    size_t headerCount = 0;

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

        if (++headerCount > MAX_HEADERS) {
            Logger::log(LogLevel::ERROR, "Too many headers in request");
            state = ParseState::ERROR;
            return false;
        }

        std::string line = buffer.substr(0, endPos);
        buffer.erase(0, endPos + 2);

        static const std::regex headerRegex(R"(^[!#$%&'*+\-.^_`|~0-9A-Za-z]+:[ \t]*[^\r\n]*$)");
        if (!std::regex_match(line, headerRegex)) {
            Logger::log(LogLevel::ERROR, "Invalid header format: " + line);
            state = ParseState::ERROR;
            return false;
        }

        size_t colonPos = line.find(':');
        std::string name = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        value.erase(0, value.find_first_not_of(" \t"));

        request->headers[name] = value;
    }
}

bool HttpParser::parseBody() {
    if (chunkedTransfer) {
        std::string fullBody;

        while (true) {

            size_t sizeEndPos = buffer.find("\r\n");
            if (sizeEndPos == std::string::npos)
                return false; // Need more data

            std::string sizeHex = buffer.substr(0, sizeEndPos);

            size_t semicolonPos = sizeHex.find(';');
            if (semicolonPos != std::string::npos) {
                sizeHex = sizeHex.substr(0, semicolonPos);
            }

            unsigned long chunkSize;
            try {
                chunkSize = std::stoul(sizeHex, nullptr, 16);
            } catch (...) {
                Logger::log(LogLevel::ERROR, "Invalid chunk size format");
                state = ParseState::ERROR;
                return false;
            }

            buffer.erase(0, sizeEndPos + 2);

            if (chunkSize == 0) {
                state = ParseState::COMPLETE;
                return true;
            }

            if (buffer.length() < chunkSize + 2)
                return false;

            fullBody.append(buffer, 0, chunkSize);

            buffer.erase(0, chunkSize + 2);
        }
    }
    if (buffer.length() < contentLength)
        return false;

    request->body = buffer.substr(0, contentLength);
    buffer.erase(0, contentLength);
    state = ParseState::COMPLETE;
    return true;
}

std::optional<HttpMethod> HttpParser::stringToMethod(const std::string &method) {
    if (method == "GET") return std::make_optional(GET);
    if (method == "POST") return std::make_optional(POST);
    if (method == "PUT") return std::make_optional(PUT);
    if (method == "DELETE") return std::make_optional(DELETE);
    if (method == "PATCH") return std::make_optional(PATCH);
    if (method == "OPTIONS") return std::make_optional(OPTIONS);

    return std::nullopt;
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
