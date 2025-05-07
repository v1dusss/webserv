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

static HttpResponse handleServeFile(const std::string &path) {
    struct stat fileStat{};
    if (stat(path.c_str(), &fileStat) != 0 || S_ISDIR(fileStat.st_mode))
        return HttpResponse::html(HttpResponse::NOT_FOUND);

    if (access(path.c_str(), R_OK) != 0)
        return HttpResponse::html(HttpResponse::FORBIDDEN);

    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return HttpResponse::html(HttpResponse::NOT_FOUND);

    HttpResponse response(HttpResponse::StatusCode::OK);
    response.enableChunkedEncoding(fd);
    response.setHeader("Content-Type", RequestHandler::getMimeType(path));
    return response;
}

// TODO: maybe we have to use poll here I don't know
static HttpResponse handleAutoIndex(const std::string &path) {
    Logger::log(LogLevel::DEBUG, "Auto indexing path: " + path);
    DIR *dir = opendir(path.c_str());
    if (!dir)
        return HttpResponse::html(HttpResponse::NOT_FOUND);

    if (access(path.c_str(), R_OK) == -1)
        return HttpResponse::html(HttpResponse::StatusCode::FORBIDDEN);

    std::vector<std::string> entries;
    dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == ".") continue;
        entries.push_back(name + (entry->d_type == DT_DIR ? "/" : ""));
    }
    closedir(dir);

    std::sort(entries.begin(), entries.end());

    std::ostringstream html;
    html << "<html><head><title>Index of " << path << "</title></head><body>";
    html << "<h1>Index of " << path << "</h1><ul>";
    for (const auto &name: entries) {
        html << "<li><a href=\"" << name << "\">" << name << "</a></li>";
    }
    html << "</ul></body></html>";

    HttpResponse response(HttpResponse::StatusCode::OK);
    response.setHeader("Content-Type", "text/html");
    response.setBody(html.str());
    return response;
}

HttpResponse RequestHandler::handleGet() const {
    struct stat fileStat{};
    if (stat(routePath.c_str(), &fileStat) != 0)
        return HttpResponse::html(HttpResponse::NOT_FOUND);

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
