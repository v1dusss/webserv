//
// Created by Emil Ebert on 01.05.25.
//

#include "HttpResponse.h"

HttpResponse::HttpResponse(int statusCode)
    : statusCode(statusCode),
      chunkedEncoding(false) {
    statusMessage = getStatusMessage(statusCode);
}

void HttpResponse::setStatus(int code, const std::string& message) {
    statusCode = code;
    statusMessage = message.empty() ? getStatusMessage(code) : message;
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    headers[name] = value;
}

void HttpResponse::setBody(const std::string& body) {
    this->body = body;
    if (!chunkedEncoding) {
        headers["Content-Length"] = std::to_string(body.length());
    }
}

void HttpResponse::enableChunkedEncoding() {
    chunkedEncoding = true;
    headers["Transfer-Encoding"] = "chunked";
    headers.erase("Content-Length");
}

void HttpResponse::addChunk(const std::string& chunk) {
    if (!chunkedEncoding) {
        enableChunkedEncoding();
    }

    std::stringstream ss;
    ss << std::hex << chunk.length() << "\r\n" << chunk << "\r\n";
    body += ss.str();
}

std::string HttpResponse::toString() const {
    std::stringstream response;

    response << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";

    for (const auto& header : headers) {
        response << header.first << ": " << header.second << "\r\n";
    }

    response << "\r\n";

    response << body;

    if (chunkedEncoding) {
        response << "0\r\n\r\n";
    }

    return response.str();
}

std::string HttpResponse::getStatusMessage(int code) const {
    switch (code) {
        case OK: return "OK";
        case CREATED: return "Created";
        case NO_CONTENT: return "No Content";
        case MOVED_PERMANENTLY: return "Moved Permanently";
        case BAD_REQUEST: return "Bad Request";
        case NOT_FOUND: return "Not Found";
        case METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case NOT_IMPLEMENTED: return "Not Implemented";
        default: return "Unknown";
    }
}

std::string HttpResponse::getBody() const {
    return body;
}