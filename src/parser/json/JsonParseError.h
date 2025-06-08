//
// Created by Emil Ebert on 08.06.25.
//

#ifndef JSONPARSEERROR_H
#define JSONPARSEERROR_H

#include <exception>
#include <string>

class JsonParseError : public std::exception {
private:
    std::string message;
    int line;
    int column;

public:
    JsonParseError(const std::string &message, int line = 0, int column = 0);

    const char *what() const throw();

    int getLine() const;

    int getColumn() const;
};


#endif //JSONPARSEERROR_H
