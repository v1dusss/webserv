//
// Created by Emil Ebert on 01.05.25.
//

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <unordered_map>
#include "config/config.h"
#include <iostream>

class HttpRequest {
public:
    HttpMethod method;
    std::string uri;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    HttpRequest() : method(GET) {
    }

    [[nodiscard]] std::string getMethodString() const {
        switch (method) {
            case GET: return "GET";
            case POST: return "POST";
            case PUT: return "PUT";
            case DELETE: return "DELETE";
            case PATCH: return "PATCH";
            case HEAD: return "HEAD";
            case OPTIONS: return "OPTIONS";
            default: return "UNKNOWN";
        }
    }

    [[nodiscard]] std::string getQueryString() const {
        size_t pos = uri.find('?');
        if (pos != std::string::npos) {
            return uri.substr(pos + 1);
        }
        return "";
    }

    [[nodiscard]] std::string getPath() const {
        size_t pos = uri.find('?');
        if (pos != std::string::npos) {
            return uri.substr(0, pos);
        }
        return uri;
    }

    [[nodiscard]] std::string getHeader(const std::string &name) const {
        auto it = headers.find(name);
        return it != headers.end() ? it->second : "";
    }

    void printRequest() const {
        std::cout << "Method: " << method << "\n"
                << "URI: " << uri << "\n"
                << "Version: " << version << "\n"
                << "Headers:\n";
        for (const auto &header: headers) {
            std::cout << header.first << ": " << header.second << "\n";
        }
        std::cout << "Body: " << body;
    }
};

#endif //HTTPREQUEST_H
