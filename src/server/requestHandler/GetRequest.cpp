#include "RequestHandler.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <map>

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

static std::string getFileExtension(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    return path.substr(dot);
}

static std::string getMimeType(const std::string& path) {
    std::string ext = getFileExtension(path);
    if (mimeTypes.count(ext)) return mimeTypes[ext];
    return "application/octet-stream";
}

HttpResponse RequestHandler::handleGet() {
    struct stat fileStat;
    if (stat(routePath.c_str(), &fileStat) != 0 || S_ISDIR(fileStat.st_mode)) {
        HttpResponse response(HttpResponse::StatusCode::NOT_FOUND);
        response.setBody("404 Not Found");
        response.setHeader("Content-Type", "text/plain");
        return response;
    }

    std::ifstream file(routePath, std::ios::binary);
    if (!file) {
        HttpResponse response(HttpResponse::StatusCode::NOT_FOUND);
        response.setBody("404 Not Found");
        response.setHeader("Content-Type", "text/plain");
        return response;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string body = ss.str();

    HttpResponse response(HttpResponse::StatusCode::OK);
    response.setBody(body);
    response.setHeader("Content-Type", getMimeType(routePath));
    return response;
}