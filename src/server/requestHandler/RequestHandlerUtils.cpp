//
// Created by Emil Ebert on 02.05.25.
//

#include "RequestHandler.h"
#include <sys/stat.h>
#include <map>
#include <dirent.h>
#include <vector>
#include <algorithm>

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

std::string RequestHandler::getFileExtension(const std::string &path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    return path.substr(dot);
}

std::string RequestHandler::getMimeType(const std::string &path) {
    std::string ext = getFileExtension(path);
    if (mimeTypes.count(ext)) return mimeTypes[ext];
    return "application/octet-stream";
}
