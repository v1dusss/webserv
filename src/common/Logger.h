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

    static void log(LogLevel level, const std::string &message) {
        if (level >= currentLogLevel) {
            std::string prefix;
            switch (level) {
                case LogLevel::INFO:    prefix = GREEN "[INFO] "; break;
                case LogLevel::WARNING: prefix = YELLOW "[WARNING] "; break;
                case LogLevel::ERROR:   prefix = RED "[ERROR] "; break;
                case LogLevel::DEBUG:   prefix = CYAN "[DEBUG] "; break;
            }
            std::cout << prefix << message << RESET << std::endl;
        }
    }
};

#endif //LOGGER_H