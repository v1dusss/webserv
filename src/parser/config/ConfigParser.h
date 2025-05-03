#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include "config/config.h"
#include "ConfigBlock.h"

class ConfigParser {
public:
    ConfigParser();

    bool parse(const std::string& filename);

    std::vector<ServerConfig> getServerConfigs() const;

private:
    ConfigBlock rootBlock;
    int currentLine;
    std::string currentFilename;
    bool parseSuccessful;

    std::vector<std::string> serverDirectives;
    std::vector<std::string> locationDirectives;
    std::vector<std::string> validDirectivePrefixes;

    bool isValidDirective(const std::string& directive, const std::string& blockType) const;
    bool validateDigitsOnly(const std::string& value, const std::string& directive);
    bool validateListenValue(const std::string& value);


    ServerConfig parseServerBlock(const ConfigBlock& block) const;

    RouteConfig parseRouteBlock(const ConfigBlock& block, const ServerConfig& serverConfig) const;

    bool parseBlock(std::ifstream& file, ConfigBlock& block);

    void parseDirective(const std::string& line, ConfigBlock& block);

    std::vector<std::string> tokenize(const std::string& str);

    std::string removeComment(const std::string& line);

    std::string trim(const std::string& str);

    bool tryParseInt(const std::string& value, int& result) const;

    void reportError(const std::string& message) const;
};

#endif // CONFIG_PARSER_H