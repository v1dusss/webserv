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


static std::string formatSize(off_t sizeInBytes) {
    const char *units[] = {"Bytes", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    const int maxUnitIndex = sizeof(units) / sizeof(units[0]) - 1;

    if (sizeInBytes < 0) {
        return "Invalid size";
    } else if (sizeInBytes == 0) {
        return "0 Bytes";
    }

    double size = static_cast<double>(sizeInBytes);
    int unitIndex = 0;

    while (size >= 1024.0 && unitIndex < maxUnitIndex) {
        size /= 1024.0;
        ++unitIndex;
    }

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2);
    stream << size;

    std::string sizeStr = stream.str();

    // Trim trailing ".00" or ".0"
    if (sizeStr.find('.') != std::string::npos) {
        // Remove ".00" or ".0" at the end
        sizeStr.erase(sizeStr.find_last_not_of('0') + 1);
        if (sizeStr.back() == '.') {
            sizeStr.pop_back();
        }
    }

    return sizeStr + " " + units[unitIndex];
}

HttpResponse RequestHandler::handleAutoIndex(const std::string &path) {
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
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\" />\n";
    html << "  <title>Directory Listing</title>\n";
    html << "  <script src=\"https://cdn.tailwindcss.com\"></script>\n";
    html << "</head>\n";
    html << "<body class=\"min-h-screen flex flex-col bg-gray-50\">\n";
    html << "  <main class=\"flex-grow container mx-auto px-16 py-16 flex flex-col items-center justify-center\">\n";
    html << "    <h2 class=\"text-4xl font-extrabold text-gray-900 mb-2\">Directory Listing</h2>\n";
    html << "    <p class=\"text-lg text-gray-600 mb-8 text-center\">\n";
    html << "      Below is the list of files and directories in the current directory.\n";
    html << "    </p>\n";
    html <<
            "    <table class=\"min-w-full text-sm divide-y divide-gray-300 shadow bg-white rounded-lg overflow-hidden\">\n";
    html << "      <thead class=\"bg-gray-100\">\n";
    html << "        <tr>\n";
    html << "          <th class=\"text-left px-6 py-3 font-semibold\">Name</th>\n";
    html << "          <th class=\"text-left px-6 py-3 font-semibold\">Last Modified</th>\n";
    html << "          <th class=\"text-left px-6 py-3 font-semibold\">Size</th>\n";
    html << "        </tr>\n";
    html << "      </thead>\n";

    for (std::vector<Entry>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        const std::string &name = it->first;
        const struct stat &info = it->second;

        std::ostringstream sizeStr;
        if (S_ISDIR(info.st_mode))
            sizeStr << "-";
        else
            sizeStr << formatSize(info.st_size);

        char timebuf[64];
        std::tm *mtime = std::localtime(&info.st_mtime);
        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M", mtime);

        html << "      <tbody class=\"divide-y divide-gray-200\">\n";
        html << "      <tr>";
        html << "  <td class=\"px-6 py-4\"><a href=\"" << request->getPath() << "/" << name <<
                "\" class=\"text-indigo-600 hover:underline\">" << name << "</a></td>";
        html << "  <td class=\"px-6 py-4 text-gray-500\">" << timebuf << "</td>";
        html << "  <td class=\"px-6 py-4 text-gray-500\">" << sizeStr.str() << "</td>";
        html << "  </tr>\n";
    }

    html << "</table>\n";
    html << "</body>\n</html>";

    HttpResponse response(HttpResponse::StatusCode::OK);
    response.setHeader("Content-Type", "text/html");
    response.setBody(html.str());
    return response;
}
