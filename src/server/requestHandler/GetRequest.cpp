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
    response.enableChunkedEncoding(std::make_shared<SmartBuffer>(fd));
    response.setHeader("Content-Type", RequestHandler::getMimeType(path));
    return response;
}

static HttpResponse handleAutoIndex(const std::string &path) {
    Logger::log(LogLevel::DEBUG, "Auto indexing path: " + path);

    if (access(path.c_str(), R_OK) == -1)
        return HttpResponse::html(HttpResponse::StatusCode::FORBIDDEN);

    DIR *dir = opendir(path.c_str());
    if (!dir)
        return HttpResponse::html(HttpResponse::StatusCode::NOT_FOUND);

    typedef std::pair<std::string, struct stat> Entry;
    std::vector<Entry> entries;
    struct dirent *entry;

    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == ".") continue;

        std::string fullPath = path + "/" + name;
        struct stat fileStat;
        if (stat(fullPath.c_str(), &fileStat) == -1)
            continue;

        if (S_ISDIR(fileStat.st_mode))
            name += "/";

        entries.push_back(std::make_pair(name, fileStat));
    }
    closedir(dir);

    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
        return a.first < b.first;
    });

    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <title>Index of " << path << "</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: sans-serif; padding: 20px; background: #f9f9f9; }\n";
    html << "    h1 { border-bottom: 1px solid #ccc; }\n";
    html << "    table { width: 100%; border-collapse: collapse; margin-top: 1em; }\n";
    html << "    th, td { padding: 8px; border-bottom: 1px solid #ddd; text-align: left; }\n";
    html << "    th { background-color:rgba(0, 0, 0, 0.4); }\n";
    html << "    tr:nth-child(odd) { background-color:rgba(0, 0, 0, 0.05); }\n";
    html << "    a { text-decoration: none; color: #0366d6; }\n";
    html << "    a:hover { text-decoration: underline; }\n";
    html << "  </style>\n";
    html << "</head>\n<body>\n";
    html << "<h1>Index of " << path << "</h1>\n";
    html << "<table>\n";
    html << "  <tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\n";

    for (std::vector<Entry>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        const std::string &name = it->first;
        const struct stat &info = it->second;

        std::ostringstream sizeStr;
        if (S_ISDIR(info.st_mode))
            sizeStr << "-";
        else
            sizeStr << info.st_size << " Bytes";

        char timebuf[64];
        std::tm *mtime = std::localtime(&info.st_mtime);
        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M", mtime);

        html << "  <tr>";
        html << "<td><a href=\"" << name << "\">" << name << "</a></td>";
        html << "<td>" << sizeStr.str() << "</td>";
        html << "<td>" << timebuf << "</td>";
        html << "</tr>\n";
    }

    html << "</table>\n";
    html << "</body>\n</html>";

    HttpResponse response(HttpResponse::StatusCode::OK);
    response.setHeader("Content-Type", "text/html");
    response.setBody(html.str());
    return response;
}

HttpResponse RequestHandler::handleGet() const {
    // if client asked for ?autoindex, always return a real directory listing
    // TODO: I don't really understand why we need this
    /*
    if (!isFile && request.getQueryString() == "autoindex") {
        Logger::log(LogLevel::DEBUG, "Forced autoindex for " + routePath);
        return handleAutoIndex(routePath);
    }
    */
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
