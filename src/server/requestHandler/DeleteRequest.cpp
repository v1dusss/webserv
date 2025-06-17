#include "RequestHandler.h"
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include <common/SessionManager.h>
#include <server/ClientConnection.h>

HttpResponse RequestHandler::handleDelete() const {
    if (routePath.back() == '/' || std::filesystem::is_directory(routePath))
        return HttpResponse::html(HttpResponse::StatusCode::CONFLICT,
                                        "Cannot delete a directory");

    struct stat fileStat{};
    if (stat(routePath.c_str(), &fileStat) != 0 || S_ISDIR(fileStat.st_mode))
        return HttpResponse::html(HttpResponse::NOT_FOUND);

    const std::string absolutePath = std::filesystem::absolute(routePath).lexically_normal().string();
    if (!SessionManager::ownsFile(client->sessionId, absolutePath))
        return HttpResponse::html(HttpResponse::StatusCode::FORBIDDEN,
                                        "You do not own this file");



    if (access(routePath.c_str(), W_OK | X_OK) != 0)
        return HttpResponse::html(HttpResponse::StatusCode::FORBIDDEN,
                                        "No write permission");

    try {
        std::filesystem::remove(routePath);
    }catch (...) {
        return HttpResponse::html(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR);

    }

    HttpResponse response(HttpResponse::StatusCode::NO_CONTENT);
    return response;
}
