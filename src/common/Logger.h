//
// Created by Emil Ebert on 01.05.25.
//

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include "../colors.h"

typedef enum {
    INFO,
    WARNING,
    ERROR,
    DEBUG
} LogLevel;

class Logger {
public:
    static LogLevel currentLogLevel;

    static void log(LogLevel level, const std::string &message) {
        if (level >= currentLogLevel) {
            std::string prefix;
            switch (level) {
                case INFO:    prefix = GREEN "[INFO] "; break;
                case WARNING: prefix = YELLOW "[WARNING] "; break;
                case ERROR:   prefix = RED "[ERROR] "; break;
                case DEBUG:   prefix = CYAN "[DEBUG] "; break;
            }
            std::cout << prefix << message << RESET << std::endl;
        }
    }
};

#endif //LOGGER_H