//
// Created by Emil Ebert on 08.06.25.
//

#include "JsonParser.h"
#include <fstream>
#include <sstream>
#include <cctype>

#include "JsonParseError.h"

JsonParser::JsonParser() : position(0), line(1), column(1) {
}

char JsonParser::peek() const {
    if (position >= input.length())
        return '\0';
    return input[position];
}

char JsonParser::get() {
    if (position >= input.length())
        return '\0';

    char c = input[position++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

void JsonParser::skipWhitespace() {
    while (position < input.length() && std::isspace(input[position]))
        get();
}

std::string JsonParser::parseString() {
    get();

    std::string result;
    bool escape = false;

    while (position < input.length()) {
        char c = get();

        if (escape) {
            switch (c) {
                case '"': result += '"';
                    break;
                case '\\': result += '\\';
                    break;
                case '/': result += '/';
                    break;
                case 'b': result += '\b';
                    break;
                case 'f': result += '\f';
                    break;
                case 'n': result += '\n';
                    break;
                case 'r': result += '\r';
                    break;
                case 't': result += '\t';
                    break;
                case 'u': {
                    std::string hex;
                    for (int i = 0; i < 4; ++i) {
                        if (position >= input.length())
                            throw JsonParseError("Unterminated Unicode escape sequence", line, column);
                        hex += get();
                    }

                    int codePoint;
                    try {
                        codePoint = std::stoi(hex, nullptr, 16);
                    } catch (const std::exception &) {
                        throw JsonParseError("Invalid Unicode escape sequence", line, column);
                    }

                    if (codePoint <= 0x7F) {
                        result += static_cast<char>(codePoint);
                    } else if (codePoint <= 0x7FF) {
                        result += static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F));
                        result += static_cast<char>(0x80 | (codePoint & 0x3F));
                    } else {
                        result += static_cast<char>(0xE0 | ((codePoint >> 12) & 0x0F));
                        result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (codePoint & 0x3F));
                    }
                    break;
                }
                default:
                    throw JsonParseError("Invalid escape sequence", line, column);
            }
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else if (c == '"') {
            return result;
        } else if (c < 0x20) {
            throw JsonParseError("Control character in string", line, column);
        } else {
            result += c;
        }
    }

    throw JsonParseError("Unterminated string", line, column);
}

std::shared_ptr<JsonValue> JsonParser::parseNumber() {
    std::string numStr;

    if (peek() == '-')
        numStr += get();

    if (peek() == '0')
        numStr += get();
    else if (std::isdigit(peek())) {
        while (position < input.length() && std::isdigit(peek()))
            numStr += get();
    } else
        throw JsonParseError("Invalid number", line, column);

    if (peek() == '.') {
        numStr += get();

        if (!std::isdigit(peek()))
            throw JsonParseError("Expected digit after decimal point", line, column);

        while (position < input.length() && std::isdigit(peek()))
            numStr += get();
    }

    if (peek() == 'e' || peek() == 'E') {
        numStr += get();

        if (peek() == '+' || peek() == '-')
            numStr += get();

        if (!std::isdigit(peek()))
            throw JsonParseError("Expected digit in exponent", line, column);

        while (position < input.length() && std::isdigit(peek()))
            numStr += get();
    }

    try {
        double num = std::stod(numStr);
        return std::make_shared<JsonValue>(num);
    } catch (const std::exception &) {
        throw JsonParseError("Invalid number format", line, column);
    }
}

std::shared_ptr<JsonValue> JsonParser::parseLiteral() {
    if (input.substr(position, 4) == "null") {
        position += 4;
        column += 4;
        return std::make_shared<JsonValue>();
    }
    if (input.substr(position, 4) == "true") {
        position += 4;
        column += 4;
        return std::make_shared<JsonValue>(true);
    }
    if (input.substr(position, 5) == "false") {
        position += 5;
        column += 5;
        return std::make_shared<JsonValue>(false);
    }

    throw JsonParseError("Invalid literal", line, column);
}

std::shared_ptr<JsonValue> JsonParser::parseArray() {
    get();
    skipWhitespace();

    JsonValue::JsonArray array;

    if (peek() == ']') {
        get();
        return std::make_shared<JsonValue>(array);
    }

    while (true) {
        array.push_back(parseValue());
        skipWhitespace();

        if (peek() == ']') {
            get();
            return std::make_shared<JsonValue>(array);
        }
        if (peek() == ',') {
            get();
            skipWhitespace();
        } else
            throw JsonParseError("Expected ',' or ']'", line, column);
    }
}

std::shared_ptr<JsonValue> JsonParser::parseObject() {
    get();
    skipWhitespace();

    JsonValue::JsonObject object;

    if (peek() == '}') {
        get();
        return std::make_shared<JsonValue>(object);
    }

    while (true) {
        if (peek() != '"')
            throw JsonParseError("Expected string as object key", line, column);

        std::string key = parseString();
        skipWhitespace();

        if (peek() != ':')
            throw JsonParseError("Expected ':' after object key", line, column);
        get();
        skipWhitespace();

        object[key] = parseValue();
        skipWhitespace();

        if (peek() == '}') {
            get();
            return std::make_shared<JsonValue>(object);
        }
        if (peek() == ',') {
            get();
            skipWhitespace();
        } else
            throw JsonParseError("Expected ',' or '}'", line, column);
    }
}



