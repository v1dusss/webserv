#include "parser/config/ConfigParser.h"
#include "colors.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <unistd.h>
#include <common/Logger.h>
#include <parser/cgi/CgiParser.h>
#include <parser/http/HttpParser.h>
#include <sys/unistd.h>

ConfigParser::ConfigParser() : rootBlock{"root", {}, {}}, currentLine(0), currentFilename(""), parseSuccessful(true) {
    httpDirectives = {
        {
            .name = "client_header_timeout",
            .type = Directive::TIME,
        },
        {
            .name = "client_max_header_size",
            .type = Directive::SIZE,
        },
        {
            .name = "client_max_header_count",
            .type = Directive::COUNT,
        }
    };

    serverDirectives = {
        {
            .name = "listen",
            .type = Directive::LIST,
        },
        {
            .name = "server_name",
            .type = Directive::LIST,
            .max_arg = 10,
        },
        {
            .name = "root",
            .type = Directive::LIST,
        },
        {
            .name = "index",
            .type = Directive::LIST,
        },
        {
            .name = "client_max_body_size",
            .type = Directive::SIZE,
        },
        {
            .name = "client_body_timeout",
            .type = Directive::TIME,
        },
        {
            .name = "body_buffer_size",
            .type = Directive::SIZE,
        },
        {
            .name = "send_body_buffer_size",
            .type = Directive::SIZE,
        },
        {
            .name = "client_header_timeout",
            .type = Directive::TIME,
        },
        {
            .name = "client_max_header_size",
            .type = Directive::SIZE,
        },
        {
            .name = "client_max_header_count",
            .type = Directive::COUNT,
        },
        {
            .name = "keepalive_timeout",
            .type = Directive::TIME,
        },
        {
            .name = "keepalive_requests",
            .type = Directive::COUNT,
        },
        {
            .name = "error_page",
            .type = Directive::LIST,
            .max_arg = 2,
            .min_arg = 2,
        },
        {
            .name = "internal_api",
            .type = Directive::TOGGLE,
        },
    };

    locationDirectives = {
        {
            .name = "root",
            .type = Directive::LIST,
        },
        {
            .name = "index",
            .type = Directive::LIST,
        },
        {
            .name = "autoindex",
            .type = Directive::TOGGLE,
        },
        {
            .name = "alias",
            .type = Directive::LIST,
        },
        {
            .name = "deny",
            .type = Directive::TOGGLE,
        },
        {
            .name = "allowed_methods",
            .type = Directive::LIST,
            .max_arg = 7,
        },
        {
            .name = "error_page",
            .type = Directive::LIST,
            .min_arg = 2,
            .max_arg = 2,
        },
        {
            .name = "return",
            .type = Directive::LIST,
            .min_arg = 2,
            .max_arg = 2,
        },
        {
            .name = "cgi",
            .type = Directive::LIST,
            .min_arg = 2,
            .max_arg = 2,
        },
    };
}

bool ConfigParser::parse(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::log(LogLevel::ERROR, "Failed to open config file.");
        return false;
    }

    if (std::filesystem::is_directory(filename)) {
        Logger::log(LogLevel::ERROR, "Config file is a directory.");
        return false;
    }

    if (access(filename.c_str(), R_OK) != 0) {
        Logger::log(LogLevel::ERROR, "Config file is not readable.");
        return false;
    }

    currentFilename = filename;
    currentLine = 0;
    parseSuccessful = true;

    const bool result = parseBlock(file, rootBlock);
    return result && parseSuccessful;
}

bool ConfigParser::isValidDirective(const std::string &directive, const std::string &blockType) const {
    // Logger::log(LogLevel::DEBUG, "isValidDirective: directive = " + directive + "blockType = " + blockType);
    if (blockType == "http" &&
        std::find(httpDirectives.begin(), httpDirectives.end(), directive) != httpDirectives.end()) {
        return true;
    }

    if (blockType == "server" &&
        std::find(serverDirectives.begin(), serverDirectives.end(), directive) != serverDirectives.end()) {
        return true;
    }

    if (blockType == "location" &&
        std::find(locationDirectives.begin(), locationDirectives.end(), directive) != locationDirectives.end()) {
        return true;
    }

    for (const auto &prefix: validDirectivePrefixes) {
        if (directive.substr(0, prefix.length()) == prefix) {
            return true;
        }
    }

    return false;
}

bool ConfigParser::validateDigitsOnly(const std::string &value, const std::string &directive) {
    std::regex digitsOnly("^\\d+$");
    if (!std::regex_match(value, digitsOnly)) {
        reportError("Invalid value for " + directive + ": '" + value + "' - should contain only digits");
        parseSuccessful = false;
        return false;
    }
    return true;
}

bool ConfigParser::validateListenValue(const std::string &value) {
    size_t colonPos = value.find(':');

    if (colonPos == std::string::npos) {
        return validateDigitsOnly(value, "listen");
    }

    std::string port = value.substr(colonPos + 1);
    return validateDigitsOnly(port, "listen");
}

std::vector<ServerConfig> ConfigParser::getServerConfigs() const {
    std::vector<ServerConfig> servers;

    for (const auto &child: rootBlock.children[0].children) {
        if (child.name == "server") {
            servers.push_back(parseServerBlock(child));
        }
    }

    return servers;
}

HttpConfig ConfigParser::getHttpConfig() const {
    for (const auto &child: rootBlock.children) {
        if (child.name == "http") {
            return(parseHttpBlock(child));
        }
    }

    return HttpConfig(); // to return nothing
}

void printconfig(ServerConfig config) {
    std::cout << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << std::endl;
    std::cout << MAGENTA BOLD << "ServerConfig:" << RESET << std::endl;
    std::cout << "  Port: " << config.port << std::endl;
    std::cout << "  Host: " << config.host << std::endl;
    std::cout << "  Server Names: ";
    for (const auto &name: config.server_names) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    std::cout << "  Root: " << config.root << std::endl;
    std::cout << "  Index: " << config.index << std::endl;
    std::cout << "  Client Max Body Size: " << config.client_max_body_size << std::endl;
    std::cout << "  Client Max Header Size: " << config.headerConfig.client_max_header_size << std::endl;
    std::cout << "  Client Header Timeout: " << config.headerConfig.client_header_timeout << std::endl;
    std::cout << "  Client Body Timeout: " << config.client_body_timeout << std::endl;
    std::cout << "  Keepalive Timeout: " << config.keepalive_timeout << std::endl;
    std::cout << "  Keepalive Requests: " << config.keepalive_requests << std::endl;
    std::cout << "  Send Body Buffer Size: " << config.send_body_buffer_size << std::endl;
    std::cout << "  Body Buffer Size: " << config.body_buffer_size << std::endl;
    std::cout << "  Error Pages: " << std::endl;
    for (const auto &errorPage: config.error_pages) {
        std::cout << "\t" << errorPage.first << ": " << errorPage.second << std::endl;
    }
    std::cout << "  location: " << std::endl;
    for (const auto &route: config.routes) {
        std::cout << "\t" << route.location << ": " << route.root << std::endl;
    }
    std::cout << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << std::endl;
}

ServerConfig ConfigParser::parseServerBlock(const ConfigBlock &block) const {
    ServerConfig config;

    config.port = 80;
    config.host = "0.0.0.0";

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
    config.headerConfig.client_max_header_size = block.getSizeValue("client_max_header_size", 8192);
    config.headerConfig.client_header_timeout = block.getSizeValue("client_header_timeout", 60);
    config.headerConfig.client_max_header_count = block.getSizeValue("client_max_header_count", 10);
    config.client_body_timeout = block.getSizeValue("client_body_timeout", 60);
    config.keepalive_timeout = block.getSizeValue("keepalive_timeout", 65);
    config.keepalive_requests = block.getSizeValue("keepalive_requests", 100);
    config.send_body_buffer_size = block.getSizeValue("send_body_buffer_size", 8192);
    config.body_buffer_size = block.getSizeValue("body_buffer_size", 8192);

    const auto errorPages = block.getDirective("error_page");
    parseErrorPages(errorPages, config.error_pages);

    for (const auto &child: block.children) {
        if (child.name == "location") {
            config.routes.push_back(parseRouteBlock(child, config));
        }
    }

    printconfig(config);

    return config;
}

HttpConfig ConfigParser::parseHttpBlock(const ConfigBlock& block) const
{
    HttpConfig httpConfig;
    ClientHeaderConfig headerConfig;

    headerConfig.client_header_timeout = block.getSizeValue("client_header_timeout", 60);
    headerConfig.client_max_header_size = block.getSizeValue("client_max_header_size", 8192);
    headerConfig.client_max_header_count = block.getSizeValue("client_max_header_count", 10);

    httpConfig.headerConfig = headerConfig;
    httpConfig.max_request_line_size = block.getSizeValue("max_request_line_size", 50);

    return httpConfig;
}

static LocationType getLocationType(const std::string &modifier) {
    if (modifier == "=")
        return LocationType::EXACT;
    if (modifier == "~")
        return LocationType::REGEX;
    if (modifier == "~*")
        return LocationType::REGEX_IGNORE_CASE;
    return LocationType::PREFIX;
}

RouteConfig ConfigParser::parseRouteBlock(const ConfigBlock &block, const ServerConfig &serverConfig) const {
    RouteConfig route;

    route.autoindex = false;
    route.deny_all = false;

    const auto params = block.getDirective("_parameters");
    if (params.empty())
        return route;
    route.type = getLocationType(params[0]);

    if (params.size() == 1)
        route.location = params[0];
    else
        route.location = params[1];

    route.root = block.getStringValue("root", serverConfig.root);
    route.index = block.getStringValue("index", serverConfig.index);
    route.autoindex = (block.getStringValue("autoindex", "off") == "on");
    route.alias = block.getStringValue("alias");
    route.deny_all = (block.getStringValue("deny", "") == "all");

    const auto methods = block.getDirective("allowed_methods");
    if (!methods.empty()) {
        for (const auto &method: methods) {
            const auto httpMethod = HttpParser::stringToMethod(method);
            if (httpMethod != std::nullopt)
                route.allowedMethods.push_back(httpMethod.value());
            else
                reportError("Invalid HTTP method in allowed_methods: " + method);
        }
    } else {
        route.allowedMethods.push_back(GET);
    }

    const auto errorPages = block.getDirective("error_page");
    parseErrorPages(errorPages, route.error_pages);

    auto cgiParams = block.findDirectivesByPrefix("cgi");
    for (const auto &[key, values]: cgiParams) {
        if (values.size() >= 2) {
            route.cgi_params[values[0]] = values[1];
        }
    }

    const auto returnDir = block.getDirective("return");
    if (returnDir.size() >= 2) {
        const int statusCode = std::stoi(returnDir[0]);
        route.return_directive.first = statusCode;
        route.return_directive.second = returnDir[1];
    } else
        route.return_directive.first = -1;

    return route;
}

void ConfigParser::parseErrorPages(const std::vector<std::string> &tokens,
                                   std::map<int, std::string> &error_pages) const {
    if (tokens.size() != 2) {
        return;
    }
    const std::string errorCode = tokens[0];
    const std::string path = tokens[1];

    try {
        error_pages[std::stoi(errorCode)] = path;
    } catch (...) {
        reportError("Invalid error page format: " + path);
    }
}


bool ConfigParser::parseBlock(std::ifstream &file, ConfigBlock &block) {
    std::string line;
    bool httpBlockFound = false;

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

                if (blockType == "http" && httpBlockFound == true) {
                    reportError("Do not parse multibile http blocks");
                    parseSuccessful = false;
                    return false;
                }

                if (blockType == "http") {
                    httpBlockFound = true;
                }

                ConfigBlock childBlock;
                childBlock.name = blockType;
                tokens.erase(tokens.begin());

                if (blockType == "location") {
                    if (tokens.empty()) {
                        reportError("Location block requires a path");
                        parseSuccessful = false;
                        return false;
                    }

                    if (tokens.size() >= 3) {
                        reportError("Invalid location: too many parameters, expected 1 or 2");
                        parseSuccessful = false;
                        return false;
                    }

                    if (tokens.size() == 2) {
                        if (tokens[0] != "=" && tokens[0] != "~" && tokens[0] != "~*") {
                            reportError("Invalid location modifier: " + tokens[0] + ". Expected '=', '~', or '~*'");
                            parseSuccessful = false;
                            return false;
                        }
                    }
                }

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

    if (httpBlockFound == false) {
        reportError("Invalid cofig: http Block is missing");
        parseSuccessful = false;
        return false;
    }

    return block.name == "root";
}

void ConfigParser::parseDirective(const std::string &line, ConfigBlock &block) {
    auto tokens = tokenize(line);
    if (tokens.empty()) return;

    const std::string key = tokens[0];
    tokens.erase(tokens.begin());

    if (block.name == "server" || block.name == "location" || block.name == "http") {
        if (!isValidDirective(key, block.name)) {
            reportError("Invalid directive '" + key + "' for " + block.name + " block");
            parseSuccessful = false;
            return;
        }
    }

    // TODO: clean up this code
    if (key == "error_page") {
        if (tokens.size() != 2) {
            reportError(
                "Invalid error_page directive: " + line + " - expected format: error_page <status_code> <path>");
            parseSuccessful = false;
            return;
        }

        int statusCode;
        if (!tryParseInt(tokens[0], statusCode)) {
            reportError("Invalid status code in error_page directive: " + tokens[0]);
            parseSuccessful = false;
            return;
        }

        if (!HttpParser::isHttpStatusCode(statusCode)) {
            reportError("Invalid HTTP status code in error_page: " + std::to_string(statusCode));
            parseSuccessful = false;
            return;
        }
    } else if (key == "listen" && !tokens.empty()) {
        if (!validateListenValue(tokens[0])) {
            return;
        }
    } else if ((key == "client_max_body_size" || key == "client_max_header_size" ||
                key == "body_buffer_size" || key == "send_body_buffer_size") && !tokens.empty()) {
        const std::regex sizeRegex("^\\d+(\\.\\d+)?([kmgt]b?|[kmgt]i?bytes|bytes)?$", std::regex::icase);
        if (!std::regex_match(tokens[0], sizeRegex)) {
            reportError("Invalid value for " + key + ": " + tokens[0]);
            parseSuccessful = false;
            return;
        }
    } else if ((key == "keepalive_timeout" || key == "keepalive_requests" || key == "client_header_timeout" ||
                key == "client_body_timeout") && !tokens.
               empty()) {
        if (!validateDigitsOnly(tokens[0], key))
            return;
    }

    block.directives[key] = tokens;
}

std::vector<std::string> ConfigParser::tokenize(const std::string &str) {
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;

    for (char c: str) {
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

std::string ConfigParser::removeComment(const std::string &line) {
    size_t pos = line.find('#');
    return (pos == std::string::npos) ? line : line.substr(0, pos);
}

std::string ConfigParser::trim(const std::string &str) {
    const auto begin = str.find_first_not_of(" \t");
    if (begin == std::string::npos) return "";

    const auto end = str.find_last_not_of(" \t");
    return str.substr(begin, end - begin + 1);
}

bool ConfigParser::tryParseInt(const std::string &value, int &result) const {
    try {
        result = std::stoi(value);
        return true;
    } catch (...) {
        return false;
    }
}

void ConfigParser::reportError(const std::string &message) const {
    const std::string location = !currentFilename.empty()
                                     ? (currentFilename + ":" + std::to_string(currentLine))
                                     : "unknown location";

    Logger::log(LogLevel::ERROR, "at " + location + ": " + message);
}
