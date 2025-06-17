#ifndef CONFIG_BLOCK_H
#define CONFIG_BLOCK_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <optional>
#include "config/config.h"

struct Directive {
    std::string name;

    enum {
        SIZE,
        TIME,
        COUNT,
        TOGGLE,
        LIST,
    }type;

    size_t min_arg = 1;
    size_t max_arg = 1;

    std::function<bool(const std::vector<std::string>&)> validate = nullptr;
};

struct ConfigBlock {
    std::string name;
    std::map<std::string, std::vector<std::string>> directives;
    std::vector<ConfigBlock> children;

    [[nodiscard]] std::vector<std::string> getDirective(const std::string &key) const;

    [[nodiscard]] std::string getStringValue(std::optional<Directive> directive, const std::string& defaultValue = "") const;

    [[nodiscard]] std::map<std::string, std::vector<std::string>> findDirectivesByPrefix(const std::string& prefix) const;

    [[nodiscard]] int getIntValue(std::optional<Directive> directive, int defaultValue = 0) const;

    [[nodiscard]] size_t getSizeValue(std::optional<Directive> directive, size_t defaultValue = 0) const;

    std::vector<ConfigBlock*> findBlocks(const std::string& blockName);
};

#endif