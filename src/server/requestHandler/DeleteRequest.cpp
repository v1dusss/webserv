#include "RequestHandler.h"
#include <sys/stat.h>
#include <unistd.h>

HttpResponse RequestHandler::handleDelete() const {
    if (routePath.back() == '/')
        return HttpResponse::html(HttpResponse::StatusCode::CONFLICT,
                                        "Cannot delete a directory");

    struct stat fileStat{};
    if (stat(routePath.c_str(), &fileStat) != 0 || S_ISDIR(fileStat.st_mode))
        return HttpResponse::html(HttpResponse::NOT_FOUND);

    if (access(routePath.c_str(), W_OK) != 0)
        return HttpResponse::html(HttpResponse::StatusCode::FORBIDDEN,
                                        "No write permission");

    if (unlink(routePath.c_str()) != 0)
        return HttpResponse::html(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR);

    HttpResponse response(HttpResponse::StatusCode::NO_CONTENT);
    return response;
}
