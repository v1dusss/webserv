//
// Created by Emil Ebert on 13.05.25.
//

#include "CgiParser.h"
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <regex>
#include <webserv.h>
#include <sys/stat.h>
#include <iostream>
#include <common/Logger.h>
#include <unistd.h>
#include <fcntl.h>


CgiParser::CgiParser()
    : state(CgiParseState::HEADERS) {
    result.body = std::make_shared<SmartBuffer>();
    contentLength = -1;
}

bool CgiParser::parse(const char *data, const size_t length) {
    if (state == CgiParseState::COMPLETE || state == CgiParseState::ERROR)
        return false;

    buffer.append(data, length);

    if (state == CgiParseState::HEADERS) {
        if (!parseHeaders())
            return false;
        state = CgiParseState::BODY;
    }

    if (state == CgiParseState::BODY) {
        if (!appendToBody(buffer))
            return false;
        state = CgiParseState::COMPLETE;
    }

    return isComplete();
}

bool CgiParser::parseHeaders() {
    std::istringstream stream(buffer);
    std::string line;
    size_t pos = 0;
    std::regex headerRegex(R"(([^:]+):\s*(.*?)\s*$)");
    std::smatch matches;

    while (std::getline(stream, line)) {
        pos += line.size() + 1;

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty()) {
            buffer.erase(0, pos);
            auto it = result.headers.find("Content-Length");
            if (it != result.headers.end()) {
                contentLength = std::stol(it->second);
            }
            return true;
        }


        if (std::regex_match(line, matches, headerRegex)) {
            if (matches[1] == "Set-Cookie") {
                result.setCookies.push_back(matches[2]);
            } else
                result.headers[matches[1]] = matches[2];
        }
    }
    return false;
}

bool CgiParser::appendToBody(std::string data) {
    const bool hasEnd = data.find("\r\n") != std::string::npos;

    if (hasEnd)
        data = data.substr(0, data.find("\r\n"));

    result.body->append(data.c_str(), data.length());

    if (contentLength != -1 && result.body->getSize() >= static_cast<size_t>(contentLength)) {
        state = CgiParseState::COMPLETE;
        return true;
    }

    buffer.clear();
    return hasEnd;
}
