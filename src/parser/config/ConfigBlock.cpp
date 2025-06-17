//
// Created by Emil Ebert on 03.05.25.
//

#include "ConfigBlock.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>

std::vector<std::string> ConfigBlock::getDirective(const std::string &key) const {
    auto it = directives.find(key);
    return it != directives.end() ? it->second : std::vector<std::string>{};
}

std::string ConfigBlock::getStringValue(std::optional<Directive> directive, const std::string &defaultValue) const {
    if (!directive.has_value()) return defaultValue;
    auto values = getDirective(directive->name);
    return values.empty() ? defaultValue : values[0];
}

int ConfigBlock::getIntValue(std::optional<Directive> directive, int defaultValue) const {
    if (!directive.has_value()) return defaultValue;
    auto values = getDirective(directive->name);
    if (values.empty()) return defaultValue;
    try {
        return std::stoi(values[0]);
    } catch (...) {
        return defaultValue;
    }
}

size_t ConfigBlock::getSizeValue(std::optional<Directive> directive, size_t defaultValue) const {
    if (!directive.has_value()) return defaultValue;
    const auto values = getDirective(directive->name);
    double multiplier = 1;
    std::string numberPart;
    std::string suffixPart;
    std::unordered_map<std::string, size_t> suffixes = {
        {"kb", 1024ULL},
        {"mb", 1024ULL * 1024},
        {"gb", 1024ULL * 1024 * 1024},
        {"tb", 1024ULL * 1024 * 1024 * 1024},
        {"bytes", 1},
        {"b", 1}
    };

    if (values.empty()) return defaultValue;
    if (directive->type == Directive::SIZE) {
        std::string str = values[0];
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);

        size_t i = 0;
        while (i < str.size() && (std::isdigit(str[i]) || str[i] == '.'))
            ++i;

        numberPart = str.substr(0, i);
        suffixPart = str.substr(i);
    } else {
        numberPart = values[0];
        suffixPart = "";
    }
    try {
        double value = std::stod(numberPart);
        auto it = suffixes.find(suffixPart);
        if (it != suffixes.end()) {
            multiplier = it->second;
        } else if (!suffixPart.empty()) {
            return defaultValue;
        }
        return static_cast<size_t>(value * multiplier);
    } catch (...) {
        return defaultValue;
    }
}

std::map<std::string, std::vector<std::string> > ConfigBlock::findDirectivesByPrefix(const std::string &prefix) const {
    std::map<std::string, std::vector<std::string> > result;
    for (const auto &[key, value]: directives) {
        if (key.substr(0, prefix.length()) == prefix) {
            result[key] = value;
        }
    }
    return result;
}

std::vector<ConfigBlock *> ConfigBlock::findBlocks(const std::string &blockName) {
    std::vector<ConfigBlock *> result;
    for (auto &child: children) {
        if (child.name == blockName) {
            result.push_back(&child);
        }
    }
    return result;
}
