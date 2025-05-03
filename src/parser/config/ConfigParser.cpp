#include "parser/config/ConfigParser.h"
#include "colors.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <parser/http/HttpParser.h>

ConfigParser::ConfigParser() : rootBlock{"root", {}, {}}, currentLine(0), currentFilename(""), parseSuccessful(true) {
    serverDirectives = {
        "listen", "server_name", "root", "index", "client_max_body_size",
        "timeout", "client_header_buffer_size", "keepalive_timeout",
        "keepalive_requests", "error_page"
    };

    locationDirectives = {
        "root", "index", "autoindex", "client_max_body_size", "alias",
        "deny", "allowed_methods", "error_page", "return"
    };

    validDirectivePrefixes = {"cgi"};
}

bool ConfigParser::parse(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << RED << "Failed to open config file: " << filename << RESET << std::endl;
        return false;
    }

    currentFilename = filename;
    currentLine = 0;
    parseSuccessful = true;

    bool result = parseBlock(file, rootBlock);
    return result && parseSuccessful;
}

bool ConfigParser::isValidDirective(const std::string& directive, const std::string& blockType) const {
    if (blockType == "server" &&
        std::find(serverDirectives.begin(), serverDirectives.end(), directive) != serverDirectives.end()) {
        return true;
    }

    if (blockType == "location" &&
        std::find(locationDirectives.begin(), locationDirectives.end(), directive) != locationDirectives.end()) {
        return true;
    }

    for (const auto& prefix : validDirectivePrefixes) {
        if (directive.substr(0, prefix.length()) == prefix) {
            return true;
        }
    }

    return false;
}

bool ConfigParser::validateDigitsOnly(const std::string& value, const std::string& directive) {
    std::regex digitsOnly("^\\d+$");
    if (!std::regex_match(value, digitsOnly)) {
        reportError("Invalid value for " + directive + ": '" + value + "' - should contain only digits");
        parseSuccessful = false;
        return false;
    }
    return true;
}

bool ConfigParser::validateListenValue(const std::string& value) {
    size_t colonPos = value.find(':');

    if (colonPos == std::string::npos) {
        return validateDigitsOnly(value, "listen");
    }

    std::string port = value.substr(colonPos + 1);
    return validateDigitsOnly(port, "listen");
}

std::vector<ServerConfig> ConfigParser::getServerConfigs() const {
    std::vector<ServerConfig> servers;

    for (const auto& child : rootBlock.children) {
        if (child.name == "server") {
            servers.push_back(parseServerBlock(child));
        }
    }

    return servers;
}

}