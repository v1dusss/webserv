#include "RequestHandler.h"

#include <sys/stat.h>
#include <fstream>

HttpResponse RequestHandler::handlePost() {
    struct stat fileStat;
    bool fileExists = (stat(routePath.c_str(), &fileStat) == 0 && !S_ISDIR(fileStat.st_mode));

    std::ofstream file(routePath, std::ios::app | std::ios::binary);
    if (!file)
        return Response::customResponse(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                        "500 Internal Server Error");

    file << request.body;
    file.close();

    HttpResponse response(fileExists ? HttpResponse::StatusCode::OK : HttpResponse::StatusCode::CREATED);
    response.setBody(fileExists ? "200 OK: Data appended" : "201 Created: File created");
    response.setHeader("Content-Type", "text/plain");
    return response;
}