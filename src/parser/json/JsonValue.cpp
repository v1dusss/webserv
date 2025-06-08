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