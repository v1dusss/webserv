//
// Created by Emil Ebert on 08.06.25.
//

#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <string>
#include <stdexcept>
#include <exception>
#include <fstream>
#include <cstddef>
#include "JsonValue.h"


class JsonValue;

class JsonParser {
private:
    std::string input;
    std::size_t position;
    int line;
    int column;

    char peek() const;

    char get();

    void skipWhitespace();

    std::string parseString();

    std::shared_ptr<JsonValue> parseValue();

    std::shared_ptr<JsonValue> parseObject();

    std::shared_ptr<JsonValue> parseArray();

    std::shared_ptr<JsonValue> parseNumber();

    std::shared_ptr<JsonValue> parseLiteral();

public:
    JsonParser();

    std::shared_ptr<JsonValue> parse(const std::string &json);

    bool validate(const std::string &json, std::string &error);
};

#endif // JSONPARSER_H
