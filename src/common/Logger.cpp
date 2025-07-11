//
// Created by Emil Ebert on 01.05.25.
//

#include "Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>

LogLevel Logger::currentLogLevel = LogLevel::ERROR;

void Logger::log(const LogLevel level, const std::string& message)
{
#ifdef DEBUG_MODE
    currentLogLevel = LogLevel::DEBUG;
#endif

    if (level <= currentLogLevel)
    {
        const auto now = std::chrono::system_clock::now();
        const auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm* timeInfo = std::localtime(&time_t);

        char timeBuffer[20];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeInfo);

        std::string prefix;
        switch (level)
        {
        case LogLevel::INFO: prefix = GREEN "[INFO] ";
            break;
        case LogLevel::WARNING: prefix = YELLOW "[WARNING] ";
            break;
        case LogLevel::ERROR: prefix = RED "[ERROR] ";
            break;
        case LogLevel::DEBUG: prefix = CYAN "[DEBUG] ";
            break;
        default:
            break;
        }
        if (level == LogLevel::ERROR)
            std::cerr << BLUE "[" << timeBuffer << "]" << RESET << " " << prefix << message << RESET << std::endl;
        else
            std::cout << BLUE "[" << timeBuffer << "]" << RESET << " " << prefix << message << RESET << std::endl;
    }
}
