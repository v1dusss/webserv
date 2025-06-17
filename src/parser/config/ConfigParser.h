#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <optional>
#include "config/config.h"
#include "ConfigBlock.h"

class ConfigParser {
public:
    ConfigParser();

    bool parse(const std::string& filename);

    [[nodiscard]] std::vector<ServerConfig> getServerConfigs() const;

    [[nodiscard]] HttpConfig getHttpConfig() const;

private:
    ConfigBlock rootBlock;
    int currentLine;
    std::string currentFilename;
    bool parseSuccessful;

    std::vector<Directive> httpDirectives;
    std::vector<Directive> serverDirectives;
    std::vector<Directive> locationDirectives;
    std::vector<Directive> validDirectivePrefixes;

    std::optional<Directive> getValidDirective(const std::string key, const std::string& blockType) const;
    bool validateDigitsOnly(const std::string& value, const std::string& directive);
    bool validateErrorPage(const std::vector<std::string> &tokens);
    bool validateListenValue(const std::vector<std::string> &tokens);

    [[nodiscard]] ServerConfig parseServerBlock(const ConfigBlock& block) const;

    [[nodiscard]] RouteConfig parseRouteBlock(const ConfigBlock& block, const ServerConfig& serverConfig) const;

    [[nodiscard]] HttpConfig parseHttpBlock(const ConfigBlock& block) const;

    bool parseBlock(std::ifstream& file, ConfigBlock& block);

    void parseDirective(const std::string& line, ConfigBlock& block);

    std::vector<std::string> tokenize(const std::string& str);

    std::string removeComment(const std::string& line);

    std::string trim(const std::string& str);

    bool tryParseInt(const std::string& value, int& result) const;

    void reportError(const std::string& message) const;

    void parseErrorPages(const std::vector<std::string> &tokens, std::map<int, std::string> &error_pages) const;
};

#endif // CONFIG_PARSER_H