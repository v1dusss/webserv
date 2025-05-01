#include "RequestHandler.h"
#include <sys/stat.h>
#include <unistd.h>

HttpResponse RequestHandler::handleDelete() {
    if (routePath.back() == '/')
        return Response::customResponse(HttpResponse::StatusCode::CONFLICT,
                                        "409 Conflict: Cannot delete a directory");

    struct stat fileStat;
    if (stat(routePath.c_str(), &fileStat) != 0 || S_ISDIR(fileStat.st_mode))
        return Response::notFoundResponse();

    if (access(routePath.c_str(), W_OK) != 0)
        return Response::customResponse(HttpResponse::StatusCode::FORBIDDEN,
                                        "403 Forbidden: No write permission");

    if (unlink(routePath.c_str()) != 0)
        return Response::customResponse(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                        "500 Internal Server Error");

    HttpResponse response(HttpResponse::StatusCode::NO_CONTENT);
    return response;
}
