//
// Created by Emil Ebert on 08.06.25.
//

#ifndef JSONVALUE_H
#define JSONVALUE_H


#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>


class JsonValue;

enum class JsonValueType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
public:
    using JsonArray = std::vector<std::shared_ptr<JsonValue> >;
    using JsonObject = std::map<std::string, std::shared_ptr<JsonValue> >;

private:
    std::variant<std::nullptr_t, bool, ssize_t, std::string, JsonArray, JsonObject> value;
    JsonValueType type;

public:
    JsonValue();

    JsonValue(bool value);

    JsonValue(ssize_t value);

    JsonValue(int value);

    JsonValue(double value);

    JsonValue(const std::string &value);

    JsonValue(const char *value);

    JsonValue(const JsonArray &array);

    JsonValue(const JsonObject &object);

    JsonValueType getType() const;

    bool isNull() const;

    bool isBoolean() const;

    bool isNumber() const;

    bool isString() const;

    bool isArray() const;

    bool isObject() const;

    bool asBoolean() const;

    ssize_t asNumber() const;

    std::string asString() const;

    JsonArray asArray() const;

    JsonObject asObject() const;

    std::size_t size() const;

    std::shared_ptr<JsonValue> operator[](std::size_t index) const;

    std::shared_ptr<JsonValue> operator[](const std::string &key) const;

    bool contains(const std::string &key) const;

    std::string toString() const;
};


#endif //JSONVALUE_H
