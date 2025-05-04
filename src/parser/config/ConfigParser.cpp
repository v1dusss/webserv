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
        "root", "index", "autoindex", "alias",
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

ServerConfig ConfigParser::parseServerBlock(const ConfigBlock& block) const {
    ServerConfig config;

    config.port = 80;
    config.host = "0.0.0.0";
    config.timeout = 60;
    config.client_max_body_size = 1 * 1024 * 1024;
    config.client_max_header_size = 8192;
    config.keepalive_timeout = 65;
    config.keepalive_requests = 100;

    auto listen = block.getDirective("listen");
    if (!listen.empty()) {
        std::string listenValue = listen[0];
        size_t colonPos = listenValue.find(':');
        if (colonPos != std::string::npos) {
            config.host = listenValue.substr(0, colonPos);
            config.port = std::stoi(listenValue.substr(colonPos + 1));
        } else {
            config.port = std::stoi(listenValue);
        }
    }

    config.server_names = block.getDirective("server_name");

    config.root = block.getStringValue("root", "/var/www/html");
    config.index = block.getStringValue("index", "index.html");
    config.client_max_body_size = block.getSizeValue("client_max_body_size", 1 * 1024 * 1024);
    config.timeout = block.getIntValue("timeout", 60);
    config.client_max_header_size = block.getSizeValue("client_header_buffer_size", 8192);
    config.keepalive_timeout = block.getIntValue("keepalive_timeout", 65);
    config.keepalive_requests = block.getIntValue("keepalive_requests", 100);

    auto errorPages = block.getDirective("error_page");
    for (const auto& errorPage : errorPages) {
        std::istringstream iss(errorPage);
        std::string errorCode, path;
        while (iss >> errorCode && iss >> path) {
            try {
                config.error_pages[std::stoi(errorCode)] = path;
            } catch (...) {
            }
        }
    }

    for (const auto& child : block.children) {
        if (child.name == "location") {
            config.routes.push_back(parseRouteBlock(child, config));
        }
    }

    return config;
}

RouteConfig ConfigParser::parseRouteBlock(const ConfigBlock& block, const ServerConfig& serverConfig) const {
    RouteConfig route;

    route.autoindex = false;
    route.deny_all = false;

    auto params = block.getDirective("_parameters");
    if (!params.empty()) {
        route.location = params[0];
    }

    route.root = block.getStringValue("root", serverConfig.root);
    route.index = block.getStringValue("index", serverConfig.index);
    route.autoindex = (block.getStringValue("autoindex", "off") == "on");
    route.alias = block.getStringValue("alias");
    route.deny_all = (block.getStringValue("deny", "") == "all");

    auto methods = block.getDirective("allowed_methods");
    if (!methods.empty()) {
        for (const auto& method : methods) {
            auto httpMethod = HttpParser::stringToMethod(method);
            if (httpMethod != std::nullopt)
                route.allowedMethods.push_back(httpMethod.value());
            else
                reportError("Invalid HTTP method in allowed_methods: " + method);
        }
    } else {
        route.allowedMethods.push_back(GET);
    }

    auto errorPages = block.getDirective("error_page");
    for (const auto& errorPage : errorPages) {
        std::istringstream iss(errorPage);
        std::string errorCode, path;
        while (iss >> errorCode && iss >> path) {
            try {
                route.error_pages[std::stoi(errorCode)] = path;
            } catch (...) {
            }
        }
    }

    auto cgiParams = block.findDirectivesByPrefix("cgi");
    for (const auto& [key, values] : cgiParams) {
        if (values.size() >= 2) {
            route.cgi_params[values[0]] = values[1];
        }
    }

    auto returnDir = block.getDirective("return");
    if (returnDir.size() >= 2) {
        route.return_directive[returnDir[0]] = returnDir[1];
    }

    return route;
}

bool ConfigParser::parseBlock(std::ifstream& file, ConfigBlock& block) {
    std::string line;

    while (std::getline(file, line) && parseSuccessful) {
        currentLine++;

        line = removeComment(line);
        line = trim(line);

        if (line.empty()) continue;

        if (line == "}") {
            return true;
        }

        if (line.back() == ';') {
            parseDirective(line.substr(0, line.size() - 1), block);
            if (!parseSuccessful) return false;
            continue;
        }

        size_t bracePos = line.find('{');
        if (bracePos != std::string::npos) {
            std::string blockHeader = trim(line.substr(0, bracePos));
            auto tokens = tokenize(blockHeader);

            if (!tokens.empty()) {
                std::string blockType = tokens[0];
                if (blockType != "server" && blockType != "location" && blockType != "http") {
                    reportError("Invalid block type: " + blockType);
                    parseSuccessful = false;
                    return false;
                }

                ConfigBlock childBlock;
                childBlock.name = blockType;
                tokens.erase(tokens.begin());

                if (!tokens.empty()) {
                    childBlock.directives["_parameters"] = tokens;
                }

                if (!parseBlock(file, childBlock)) {
                    return false;
                }

                block.children.push_back(childBlock);
            }
            continue;
        }

        reportError("Invalid configuration line: " + line);
        parseSuccessful = false;
        return false;
    }

    return block.name == "root";
}

void ConfigParser::parseDirective(const std::string& line, ConfigBlock& block) {
    auto tokens = tokenize(line);
    if (tokens.empty()) return;

    std::string key = tokens[0];
    tokens.erase(tokens.begin());

    if (block.name == "server" || block.name == "location") {
        if (!isValidDirective(key, block.name)) {
            reportError("Invalid directive '" + key + "' for " + block.name + " block");
            parseSuccessful = false;
            return;
        }
    }

    if (key == "listen" && !tokens.empty()) {
        if (!validateListenValue(tokens[0])) {
            return;
        }
    } else if (key == "client_max_body_size" && !tokens.empty()) {
        std::regex sizeRegex("^\\d+[kmg]?$", std::regex::icase);
        if (!std::regex_match(tokens[0], sizeRegex)) {
            reportError("Invalid value for client_max_body_size: " + tokens[0]);
            parseSuccessful = false;
            return;
        }
    } else if ((key == "keepalive_timeout" || key == "keepalive_requests") && !tokens.empty()) {
        if (!validateDigitsOnly(tokens[0], key)) {
            return;
        }
    }

    block.directives[key] = tokens;
}

std::vector<std::string> ConfigParser::tokenize(const std::string& str) {
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;

    for (char c : str) {
        if (c == '"') {
            inQuotes = !inQuotes;
            token += c;
        } else if ((c == ' ' || c == '\t') && !inQuotes) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }

    if (!token.empty()) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string ConfigParser::removeComment(const std::string& line) {
    size_t pos = line.find('#');
    return (pos == std::string::npos) ? line : line.substr(0, pos);
}

std::string ConfigParser::trim(const std::string& str) {
    const auto begin = str.find_first_not_of(" \t");
    if (begin == std::string::npos) return "";

    const auto end = str.find_last_not_of(" \t");
    return str.substr(begin, end - begin + 1);
}

bool ConfigParser::tryParseInt(const std::string& value, int& result) const {
    try {
        result = std::stoi(value);
        return true;
    } catch (...) {
        return false;
    }
}

void ConfigParser::reportError(const std::string& message) const {
    std::string location = !currentFilename.empty() ?
        (currentFilename + ":" + std::to_string(currentLine)) :
        "unknown location";

    std::cerr << RED << "Error at " << location << ": " << message << RESET << std::endl;
}