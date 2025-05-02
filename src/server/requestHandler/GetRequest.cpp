#include "RequestHandler.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <map>
#include <dirent.h>
#include <vector>
#include <algorithm>
#include <iostream>

static HttpResponse handleServeFile(const std::string &path) {
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) != 0 || S_ISDIR(fileStat.st_mode))
        return Response::notFoundResponse();

    std::ifstream file(path, std::ios::binary);
    if (!file)
        return Response::notFoundResponse();

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string body = ss.str();

    HttpResponse response(HttpResponse::StatusCode::OK);
    response.setBody(body);
    response.setHeader("Content-Type", RequestHandler::getMimeType(path));
    return response;
}

static HttpResponse handleAutoIndex(const std::string &path) {
    DIR *dir = opendir(path.c_str());
    if (!dir)
        return Response::notFoundResponse();

    std::vector<std::string> entries;
    struct dirent *entry;
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

HttpResponse RequestHandler::handleGet() {
    struct stat fileStat;
    if (stat(routePath.c_str(), &fileStat) != 0)
        return Response::notFoundResponse();

    if (!isFile) {
        if (hasValidIndexFile)
            return handleServeFile(indexFilePath);

        if (!matchedRoute->autoindex)
            return Response::notFoundResponse();
        return handleAutoIndex(routePath);
    }

    return handleServeFile(routePath);
}
