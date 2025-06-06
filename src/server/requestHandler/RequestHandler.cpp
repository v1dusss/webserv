//
// Created by Emil Ebert on 01.05.25.
//

#include "RequestHandler.h"

#include <unistd.h>
#include <sys/stat.h>
#include <filesystem>
#include <fcntl.h>
#include <algorithm>
#include <fstream>
#include <regex>
#include <csignal>
#include <string>


#include <sys/types.h>
#include <arpa/inet.h>
#include <server/CallbackHandler.h>
#include <server/FdHandler.h>
#include <sys/poll.h>

#include "common/Logger.h"
#include "webserv.h"
#include "server/ClientConnection.h"

// Dashboard
#include <nlohmann/json.hpp>
#ifdef __linux__
#  include <sys/sysinfo.h>
#  include <sys/statvfs.h>
#else
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/sysctl.h>
#  include <mach/mach.h>
#  include <sys/mount.h>
#endif



RequestHandler::RequestHandler(ClientConnection *connection, const std::shared_ptr<HttpRequest>& request,
                               ServerConfig &serverConfig): request(request), client(connection),
                                                            serverConfig(serverConfig) {
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(connection->clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    Logger::log(LogLevel::DEBUG, "IP: " + std::string(ipStr) +
                                 " Port: " + std::to_string(ntohs(connection->clientAddr.sin_port)) + " request: " +
                                 request->getMethodString() + " at " + request->uri);

    findRoute();
    setRoutePath();
}

RequestHandler::~RequestHandler() {
    if (cgiInputFd != -1) {
        FdHandler::removeFd(cgiInputFd);
        close(cgiInputFd);
        cgiInputFd = -1;
    }
    if (cgiOutputFd != -1) {
        FdHandler::removeFd(cgiOutputFd);
        close(cgiOutputFd);
        cgiOutputFd = -1;
    }
    if (fileWriteFd != -1) {
        FdHandler::removeFd(fileWriteFd);
        close(fileWriteFd);
        fileWriteFd = -1;
    }
    if (cgiProcessId != -1) {
        kill(cgiProcessId, SIGINT);
    }
    if (postRequestCallbackId != -1) {
        CallbackHandler::unregisterCallback(postRequestCallbackId);
        postRequestCallbackId = -1;
    }
}


void RequestHandler::findRoute() {
    size_t longestMatch = 0;
    for (const RouteConfig &route: serverConfig.routes) {
        if (route.type == LocationType::EXACT && request->getPath() == route.location) {
            matchedRoute = route;
            longestMatch = route.location.length();
            break;
        }
        if (route.type == LocationType::REGEX &&
            std::regex_match(request->getPath(), std::regex(route.location))) {
            matchedRoute = route;
            longestMatch = route.location.length();
            break;
        }
        if (route.type == LocationType::PREFIX && request->getPath().find(route.location) == 0) {
            if (route.location.length() > longestMatch) {
                matchedRoute = route;
                longestMatch = route.location.length();
            }
        }
    }
    if (longestMatch > 0) {
        Logger::log(LogLevel::DEBUG, "Matched route: " + matchedRoute->location);
        return;
    }

    matchedRoute.reset();
    Logger::log(LogLevel::WARNING, "No matching route found for URI: " + request->uri);
}

void RequestHandler::setRoutePath() {
    if (!matchedRoute.has_value()) {
        routePath.clear();
        return;
    }

    const RouteConfig &route = matchedRoute.value();
    std::string basePath;

    if (!route.alias.empty()) {
        basePath = route.alias;
    } else if (!route.root.empty()) {
        basePath = route.root;
    } else {
        basePath = serverConfig.root;
    }

    std::string uriSuffix = request->getPath().substr(route.location.length());
    if (!uriSuffix.empty() && uriSuffix[0] == '/') {
        uriSuffix.erase(0, 1);
    }

    if (!basePath.empty() && basePath.back() != '/')
        basePath += '/';
    routePath = basePath + uriSuffix;
    Logger::log(LogLevel::DEBUG, "set path to " + routePath);
}

void RequestHandler::validateTargetPath() {
    isFile = std::filesystem::is_regular_file(routePath);
    if (isFile)
        return;

    const RouteConfig route = matchedRoute.value();
    if (route.index.empty() && serverConfig.index.empty())
        return;

    const std::string indexFilePath = std::filesystem::path(routePath) / (!route.index.empty()
                                                                              ? route.index
                                                                              : serverConfig.index);

    hasValidIndexFile = std::filesystem::exists(indexFilePath) && std::filesystem::is_regular_file(indexFilePath) &&
                        access(indexFilePath.c_str(), R_OK) == 0;

    this->indexFilePath = indexFilePath;
}

bool RequestHandler::isCgiRequest() {
    if (matchedRoute->cgi_params.empty())
        return false;

    const std::string filePath = getFilePath();

    const std::string fileExtension = getFileExtension(filePath);
    if (fileExtension.empty())
        return false;

    if (matchedRoute->cgi_params.count(fileExtension) == 0)
        return false;

    const std::string cgiPath = matchedRoute->cgi_params.at(fileExtension);
    if (cgiPath.empty())
        return false;

    this->cgiPath = cgiPath;
    return true;
}

void RequestHandler::execute() {
    const auto response = handleRequest();
    if (response.has_value()) {
        client->setResponse(handleCustomErrorPage(response.value(), serverConfig, matchedRoute));
    }
}

std::optional<HttpResponse> RequestHandler::handleMetrics() const {
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) != 0)
        return HttpResponse::html(HttpResponse::INTERNAL_SERVER_ERROR, "sysinfo failed");
    double load1    = si.loads[0]   / double(1 << SI_LOAD_SHIFT);
    uint64_t totalRam = si.totalram * si.mem_unit;
    uint64_t freeRam  = si.freeram  * si.mem_unit;
    uint64_t usedRam  = totalRam - freeRam;
    struct statvfs fs;
    if (statvfs(serverConfig.root.c_str(), &fs) != 0)
        return HttpResponse::html(HttpResponse::INTERNAL_SERVER_ERROR, "statvfs failed");
    uint64_t totalDisk = fs.f_blocks * fs.f_frsize;
    uint64_t freeDisk  = fs.f_bfree  * fs.f_frsize;
    uint64_t usedDisk  = totalDisk - freeDisk;
#else
    // CPU load
    double loads[1] = {0};
    if (getloadavg(loads, 1) == -1)
        return HttpResponse::html(HttpResponse::INTERNAL_SERVER_ERROR, "getloadavg failed");
    double load1 = loads[0];

    // Memory
    uint64_t totalRam = 0, freeRam = 0, usedRam = 0;
    size_t len = sizeof(totalRam);
    if (sysctlbyname("hw.memsize", &totalRam, &len, NULL, 0) != 0)
        return HttpResponse::html(HttpResponse::INTERNAL_SERVER_ERROR, "sysctl hw.memsize failed");

    mach_port_t host = mach_host_self();
    vm_size_t pageSize;
    if (host_page_size(host, &pageSize) != KERN_SUCCESS)
        return HttpResponse::html(HttpResponse::INTERNAL_SERVER_ERROR, "host_page_size failed");

    vm_statistics64_data_t vmStat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(host, HOST_VM_INFO64,
                         reinterpret_cast<host_info64_t>(&vmStat),
                         &count) != KERN_SUCCESS)
        return HttpResponse::html(HttpResponse::INTERNAL_SERVER_ERROR, "host_statistics failed");

    freeRam = uint64_t(vmStat.free_count) * pageSize;
    usedRam = totalRam - freeRam;

    // Disk
    struct statfs fs;
    // try configured root, else fall back to '/' mount
    if (statfs(serverConfig.root.c_str(), &fs) != 0) {
        if (statfs("/", &fs) != 0) {
            return HttpResponse::html(HttpResponse::INTERNAL_SERVER_ERROR, "statfs failed");
        }
    }
    uint64_t totalDisk = uint64_t(fs.f_blocks) * fs.f_bsize;
    uint64_t freeDisk  = uint64_t(fs.f_bfree)  * fs.f_bsize;
    uint64_t usedDisk  = totalDisk - freeDisk;
#endif

    // build JSON response
    std::ostringstream js;
    js << "{"
       << "\"cpu_load\":" << load1 << ","
       << "\"memory\":{"
          << "\"total\":" << totalRam << ","
          << "\"used\":"  << usedRam  << "},"
       << "\"disk\":{"
          << "\"total\":" << totalDisk << ","
          << "\"used\":"  << usedDisk  << "}"
       << "}";
    auto body = js.str();

    HttpResponse resp(HttpResponse::OK);
    resp.setHeader("Content-Type", "application/json");
    resp.setHeader("Content-Length", std::to_string(body.size()));
    resp.setBody(body);
    return resp;
}

std::optional<HttpResponse> RequestHandler::handleRequest() {
    auto path = request->getPath();
    if (path == "/dashboard/metrics") {
        return handleMetrics();
    }

    if (!matchedRoute.has_value())
        return HttpResponse::html(HttpResponse::NOT_FOUND);

    if (std::find(matchedRoute->allowedMethods.begin(), matchedRoute->allowedMethods.end(), request->method) ==
        matchedRoute->allowedMethods.end()) {
        return HttpResponse::html(HttpResponse::StatusCode::METHOD_NOT_ALLOWED);
    }


    validateTargetPath();

    if (isCgiRequest()) {
        Logger::log(LogLevel::DEBUG, "request is a CGI request");

        return handleCgi();
    }

    switch (request->method) {
        case GET:
            return handleGet();
        case POST:
            return handlePost();
        case PUT:
            return handlePut();
        case DELETE:
            return handleDelete();
        default:
            return HttpResponse::html(HttpResponse::StatusCode::METHOD_NOT_ALLOWED);
    }
}

HttpResponse RequestHandler::handleCustomErrorPage(HttpResponse original, ServerConfig &serverConfig,
                                                   std::optional<RouteConfig> matchedRoute) {
    std::string errorPagePath;

    if (matchedRoute.has_value() && matchedRoute->error_pages.count(original.getStatus()))
        errorPagePath = matchedRoute->error_pages[original.getStatus()];
    else if (serverConfig.error_pages.count(original.getStatus()))
        errorPagePath = serverConfig.error_pages[original.getStatus()];
    else
        return original;


    if (!std::filesystem::is_regular_file(errorPagePath) || access(errorPagePath.c_str(), R_OK) != 0) {
        Logger::log(LogLevel::ERROR, "error page has an invalid path: " + errorPagePath);
        return original;
    }
    const int fd = open(errorPagePath.c_str(), O_RDONLY);
    if (fd < 0) {
        Logger::log(LogLevel::ERROR, "error page does not exist: " + errorPagePath);
        return original;
    }

    /*
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;

    if (const int poll_result = poll(&pfd, 1, READ_FILE_TIMEOUT); poll_result <= 0) {
        if (poll_result == 0)
            Logger::log(LogLevel::ERROR, "Timeout reading error page: " + errorPagePath);
        else
            Logger::log(LogLevel::ERROR, "Poll failed for error page: " + errorPagePath);
        close(fd);
        return original;
    }


    std::ostringstream ss;
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        ss.write(buffer, bytes_read);
    }
    close(fd);
    const std::string body = ss.str();
    */

    HttpResponse newResponse(HttpResponse::StatusCode::OK);
    newResponse.enableChunkedEncoding(std::make_shared<SmartBuffer>(fd));
    newResponse.setHeader("Content-Type", getMimeType(errorPagePath));
    newResponse.setStatus(original.getStatus());
    return newResponse;
}


std::string RequestHandler::getFilePath() const {
    if (hasValidIndexFile)
        return this->indexFilePath;
    return this->routePath;
}
