//
// Created by Emil Ebert on 08.06.25.
//

#include "JsonValue.h"
#include <string>
#include <stdexcept>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>

#include "JsonParseError.h"
#include "JsonParser.h"


JsonValue::JsonValue() : value(nullptr), type(JsonValueType::Null) {
}

JsonValue::JsonValue(bool val) : value(val), type(JsonValueType::Boolean) {
}

JsonValue::JsonValue(int val) : value(static_cast<double>(val)), type(JsonValueType::Number) {
}

JsonValue::JsonValue(double val) : value(val), type(JsonValueType::Number) {
}

JsonValue::JsonValue(const std::string &val) : value(val), type(JsonValueType::String) {
}

JsonValue::JsonValue(const char *val) : value(std::string(val)), type(JsonValueType::String) {
}

JsonValue::JsonValue(const JsonArray &arr) : value(arr), type(JsonValueType::Array) {
}

JsonValue::JsonValue(const JsonObject &obj) : value(obj), type(JsonValueType::Object) {
}

JsonValueType JsonValue::getType() const { return type; }
bool JsonValue::isNull() const { return type == JsonValueType::Null; }
bool JsonValue::isBoolean() const { return type == JsonValueType::Boolean; }
bool JsonValue::isNumber() const { return type == JsonValueType::Number; }
bool JsonValue::isString() const { return type == JsonValueType::String; }
bool JsonValue::isArray() const { return type == JsonValueType::Array; }
bool JsonValue::isObject() const { return type == JsonValueType::Object; }

bool JsonValue::asBoolean() const {
    if (!isBoolean())
        throw JsonParseError("JSON value is not a boolean");
    return std::get<bool>(value);
}

double JsonValue::asNumber() const {
    if (!isNumber())
        throw JsonParseError("JSON value is not a number");
    return std::get<double>(value);
}

std::string JsonValue::asString() const {
    if (!isString())
        throw JsonParseError("JSON value is not a string");
    return std::get<std::string>(value);
}

JsonValue::JsonArray JsonValue::asArray() const {
    if (!isArray())
        throw JsonParseError("JSON value is not an array");
    return std::get<JsonArray>(value);
}

JsonValue::JsonObject JsonValue::asObject() const {
    if (!isObject())
        throw JsonParseError("JSON value is not an object");
    return std::get<JsonObject>(value);
}

std::size_t JsonValue::size() const {
    if (isArray())
        return std::get<JsonArray>(value).size();
    if (isObject())
        return std::get<JsonObject>(value).size();
    return 0;
}

std::shared_ptr<JsonValue> JsonValue::operator[](std::size_t index) const {
    if (!isArray())
        throw JsonParseError("JSON value is not an array");
    const auto &arr = std::get<JsonArray>(value);
    if (index >= arr.size())
        throw std::out_of_range("Array index out of range");
    return arr[index];
}

std::shared_ptr<JsonValue> JsonValue::operator[](const std::string &key) const {
    if (!isObject())
        throw JsonParseError("JSON value is not an object");
    const auto &obj = std::get<JsonObject>(value);
    auto it = obj.find(key);
    if (it == obj.end())
        throw std::out_of_range("Key not found: " + key);
    return it->second;
}

bool JsonValue::contains(const std::string &key) const {
    if (!isObject())
        return false;
    const auto &obj = std::get<JsonObject>(value);
    return obj.find(key) != obj.end();
}

std::string JsonValue::toString() const {
    switch (type) {
        case JsonValueType::Null:
            return "null";
        case JsonValueType::Boolean:
            return std::get<bool>(value) ? "true" : "false";
        case JsonValueType::Number: {
            auto num = std::get<double>(value);
            if (std::floor(num) == num)
                return std::to_string(static_cast<long long>(num));
            return std::to_string(num);
        }
        case JsonValueType::String: {
            std::string result = "\"";
            for (char c: std::get<std::string>(value)) {
                switch (c) {
                    case '\"': result += "\\\"";
                        break;
                    case '\\': result += "\\\\";
                        break;
                    case '\b': result += "\\b";
                        break;
                    case '\f': result += "\\f";
                        break;
                    case '\n': result += "\\n";
                        break;
                    case '\r': result += "\\r";
                        break;
                    case '\t': result += "\\t";
                        break;
                    default:
                        if ( c < 32) {
                            char buf[8];
                            snprintf(buf, sizeof(buf), "\\u%04x", c);
                            result += buf;
                        } else {
                            result += c;
                        }
                }
            }
            result += "\"";
            return result;
        }
        case JsonValueType::Array: {
            std::string result = "[";
            const auto &arr = std::get<JsonArray>(value);
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) result += ",";
                result += arr[i]->toString();
            }
            result += "]";
            return result;
        }
        case JsonValueType::Object: {
            std::string result = "{";
            const auto &obj = std::get<JsonObject>(value);
            bool first = true;
            for (const auto &[key, val]: obj) {
                if (!first) result += ",";
                first = false;
                result += "\"" + key + "\":" + val->toString();
            }
            result += "}";
            return result;
        }
    }
    return "";
}
