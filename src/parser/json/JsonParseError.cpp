//
// Created by Emil Ebert on 08.06.25.
//

#include "JsonParseError.h"

JsonParseError::JsonParseError(const std::string &msg, int ln, int col)
    : message(msg), line(ln), column(col) {
}

const char *JsonParseError::what() const noexcept {
    static std::string errorMsg;
    if (line > 0 && column > 0)
        errorMsg = message + " at line " + std::to_string(line) + ", column " + std::to_string(column);
    else
        errorMsg = message;
    return errorMsg.c_str();
}

int JsonParseError::getLine() const { return line; }
int JsonParseError::getColumn() const { return column; }
