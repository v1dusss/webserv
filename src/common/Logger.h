//
// Created by Emil Ebert on 01.05.25.
//

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include "../colors.h"


enum class LogLevel{
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
    DEBUG = 3
} ;

class Logger {
public:
    static LogLevel currentLogLevel;

    static void log(LogLevel level, const std::string &message);
};

#endif //LOGGER_H