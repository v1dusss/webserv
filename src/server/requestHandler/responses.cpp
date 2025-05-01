#include "RequestHandler.h"

HttpResponse Response::notFoundResponse() {
    HttpResponse response(HttpResponse::StatusCode::NOT_FOUND);
    response.setBody("404 Not Found");
    response.setHeader("Content-Type", "text/plain");
    return response;
}

HttpResponse Response::customResponse(int statusCode, const std::string &body,
                                       const std::string &contentType) {
    HttpResponse response(statusCode);
    response.setBody(body);
    response.setHeader("Content-Type", contentType);
    return response;
}