#include "parser/config/ConfigParser.h"
#include "colors.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <parser/http/HttpParser.h>

std::vector<std::string> ConfigBlock::getDirective(const std::string& key) const {
    auto it = directives.find(key);
    return it != directives.end() ? it->second : std::vector<std::string>{};
}

std::string ConfigBlock::getStringValue(const std::string& key, const std::string& defaultValue) const {
    auto values = getDirective(key);
    return values.empty() ? defaultValue : values[0];
}

int ConfigBlock::getIntValue(const std::string& key, int defaultValue) const {
    auto values = getDirective(key);
    if (values.empty()) return defaultValue;
    try {
        return std::stoi(values[0]);
    } catch (...) {
        return defaultValue;
    }
}

std::map<std::string, std::vector<std::string>> ConfigBlock::findDirectivesByPrefix(const std::string& prefix) const {
    std::map<std::string, std::vector<std::string>> result;
    for (const auto& [key, value] : directives) {
        if (key.substr(0, prefix.length()) == prefix) {
            result[key] = value;
        }
    }
    return result;
}

std::vector<ConfigBlock*> ConfigBlock::findBlocks(const std::string& blockName) {
    std::vector<ConfigBlock*> result;
    for (auto& child : children) {
        if (child.name == blockName) {
            result.push_back(&child);
        }
    }
    return result;
}

