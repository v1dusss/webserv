//
// Created by Emil Ebert on 15.06.25.
//

#ifndef METRICHANDLER_H
#define METRICHANDLER_H

#include <string>
#include <unordered_map>
#include <iostream>

#define RESET_INTERVAL 10

class MetricHandler {
private:
    static std::unordered_map<std::string, size_t> metrics;
    static std::unordered_map<std::string, size_t> lastFullMetrics;
    static std::time_t lastResetTime;

public:
    static void incrementMetric(const std::string &metricName, size_t value);

    static std::unordered_map<std::string, size_t>& getAllFullMetric();

    static void resetMetrics();

    static std::time_t getLastResetTime();
};


#endif //METRICHANDLER_H
