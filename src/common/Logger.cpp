//
// Created by Emil Ebert on 01.05.25.
//

#include "Logger.h"

LogLevel Logger::currentLogLevel = LogLevel::INFO;

void Logger::log(const LogLevel level, const std::string &message) {
    if (level >= currentLogLevel) {
        std::string prefix;
        switch (level) {
            case LogLevel::INFO: prefix = GREEN "[INFO] ";
                break;
            case LogLevel::WARNING: prefix = YELLOW "[WARNING] ";
                break;
            case LogLevel::ERROR: prefix = RED "[ERROR] ";
                break;
            case LogLevel::DEBUG: prefix = CYAN "[DEBUG] ";
                break;
        }
        if (level == LogLevel::ERROR)
            std::cerr << prefix << message << std::endl;
        else
            std::cout << prefix << message << RESET << std::endl;
    }
}
