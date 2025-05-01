#include "RequestHandler.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <map>
#include <dirent.h>
#include <vector>
#include <algorithm>
#include <iostream>

static std::map<std::string, std::string> mimeTypes = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".txt", "text/plain"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"}
};

static std::string getFileExtension(const std::string &path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    return path.substr(dot);
}

static std::string getMimeType(const std::string &path) {
    std::string ext = getFileExtension(path);
    if (mimeTypes.count(ext)) return mimeTypes[ext];
    return "application/octet-stream";
}

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
    response.setHeader("Content-Type", getMimeType(path));
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

    if (S_ISDIR(fileStat.st_mode)) {
        if (!matchedRoute->index.empty() || !serverConfig.index.empty())
            return handleServeFile(
                routePath + (!matchedRoute->index.empty() ? matchedRoute->index : serverConfig.index));

        if (!matchedRoute->autoindex)
            return Response::notFoundResponse();
        return handleAutoIndex(routePath);
    }

    return handleServeFile(routePath);
}
