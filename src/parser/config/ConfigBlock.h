#ifndef CONFIG_BLOCK_H
#define CONFIG_BLOCK_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include "config/config.h"

struct ConfigBlock {
    std::string name;
    std::map<std::string, std::vector<std::string>> directives;
    std::vector<ConfigBlock> children;

    std::vector<std::string> getDirective(const std::string& key) const;

    std::string getStringValue(const std::string& key, const std::string& defaultValue = "") const;

    std::map<std::string, std::vector<std::string>> findDirectivesByPrefix(const std::string& prefix) const;

    int getIntValue(const std::string& key, int defaultValue = 0) const;

    size_t getSizeValue(const std::string& key, size_t defaultValue = 0) const;

    std::vector<ConfigBlock*> findBlocks(const std::string& blockName);
};

#endif