#include "RequestHandler.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <map>
#include <dirent.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <common/Logger.h>
#include <sys/fcntl.h>
#include <ctime>
#include <iomanip>
#include <string>
#include <filesystem>

static HttpResponse handleServeFile(const std::string &path) {
    if (!std::filesystem::exists(path) || std::filesystem::is_directory(path))
        return HttpResponse::html(HttpResponse::NOT_FOUND);

    if (access(path.c_str(), R_OK) != 0)
        return HttpResponse::html(HttpResponse::FORBIDDEN);

    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return HttpResponse::html(HttpResponse::NOT_FOUND);

    HttpResponse response(HttpResponse::StatusCode::OK);
    response.enableChunkedEncoding(std::make_shared<SmartBuffer>(fd));
    response.setHeader("Content-Type", RequestHandler::getMimeType(path));
    return response;
}

HttpResponse RequestHandler::handleGet() {
    if (!isFile) {
        Logger::log(LogLevel::DEBUG, "Route is a directory");
        if (hasValidIndexFile) {
            Logger::log(LogLevel::DEBUG, "Serving index file: " + indexFilePath);
            return handleServeFile(indexFilePath);
        }

        if (!matchedRoute->autoindex) {
            Logger::log(LogLevel::DEBUG, "Route Autoindex is disabled");
            return HttpResponse::html(HttpResponse::NOT_FOUND);
        }
        return handleAutoIndex(routePath);
    }

    return handleServeFile(routePath);
}
