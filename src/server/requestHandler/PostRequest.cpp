#include "RequestHandler.h"

#include <sys/stat.h>
#include <fstream>

#include <regex>

HttpResponse RequestHandler::handlePost() {
    std::string contentType = request.getHeader("Content-Type");
    std::smatch match;

    if (contentType.find("multipart/form-data") == std::string::npos) {
        return Response::customResponse(HttpResponse::StatusCode::BAD_REQUEST,
                                        "400 Bad Request: Invalid Content-Type");
    }

    std::regex boundaryRegex("boundary\\s*=\\s*([^;]+)");
    if (!std::regex_search(contentType, match, boundaryRegex)) {
        return Response::customResponse(HttpResponse::StatusCode::BAD_REQUEST,
                                        "400 Bad Request: Missing boundary");
    }
    std::string boundary = "--" + match[1].str();

    size_t partStart = request.body.find(boundary);
    if (partStart == std::string::npos)
        return Response::customResponse(HttpResponse::StatusCode::BAD_REQUEST, "400 Bad Request: Malformed multipart body");
    partStart += boundary.length() + 2;

    size_t headersEnd = request.body.find("\r\n\r\n", partStart);
    if (headersEnd == std::string::npos)
        return Response::customResponse(HttpResponse::StatusCode::BAD_REQUEST, "400 Bad Request: Malformed multipart body");
    headersEnd += 4;

    size_t partEnd = request.body.find(boundary, headersEnd);
    if (partEnd == std::string::npos)
        partEnd = request.body.size();

    std::string fileContent = request.body.substr(headersEnd, partEnd - headersEnd - 2);

    std::ofstream file(routePath, std::ios::binary);
    if (!file)
        return Response::customResponse(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                        "500 Internal Server Error");
    file.write(fileContent.c_str(), fileContent.size());
    file.close();

    HttpResponse response(HttpResponse::StatusCode::CREATED);
    response.setBody("201 Created: File uploaded");
    response.setHeader("Content-Type", "text/plain");
    return response;
}
