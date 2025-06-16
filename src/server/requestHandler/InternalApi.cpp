//
// Created by Emil Ebert on 12.06.25.
//

#include "InternalApi.h"
#include "../../parser/json/JsonParser.h"
#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/sysctl.h>
#else
#include <sys/sysinfo.h>
#endif
#include <server/ServerPool.h>
#include <server/handler/MetricHandler.h>
#include <sys/statvfs.h>
#include <common/Logger.h>

HttpResponse metrics(std::shared_ptr<HttpRequest> request) {
    (void) request;
    Logger::log(LogLevel::DEBUG, "Internal API: Metrics endpoint accessed");
    HttpResponse response(200);
    response.setHeader("Content-Type", "application/json");

    JsonValue::JsonObject jsonObj;
    jsonObj["connection_count"] = std::make_shared<JsonValue>(ServerPool::getClientCount());

    int uptimeSeconds = std::time(nullptr) - ServerPool::getStartTime();
    jsonObj["uptime"] = std::make_shared<JsonValue>(uptimeSeconds);

    jsonObj["last_update"] = std::make_shared<JsonValue>(MetricHandler::getLastResetTime());

    for (const auto&[fst, snd] : MetricHandler::getAllFullMetric())
        jsonObj[fst] = std::make_shared<JsonValue>(static_cast<ssize_t>(snd));


    auto metricsObj = std::make_shared<JsonValue>(jsonObj);

    std::stringstream ss;
    response.setBody(metricsObj->toString());
    return response;
}


void InternalApi::registerRoutes(ServerConfig &serverConfig) {
    serverConfig.routes.push_back({
        .location = "/metrics",
        .type = LocationType::PREFIX,
        .allowedMethods = {GET},
        .root = "",
        .autoindex = false,
        .index = "",
        .alias = "",
        .error_pages = {},
        .deny_all = false,
        .cgi_params = {},
        .return_directive = {-1, ""},
        .internalHandler = metrics,

    });
}
