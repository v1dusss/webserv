//
// Created by Emil Ebert on 01.05.25.
//

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H


#include <string>
#include <unordered_map>
#include <sstream>

class HttpResponse {
private:
    int statusCode;
    std::string statusMessage;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    bool chunkedEncoding;
    int bodyFd = -1;

public:
    // only used for chunked encoding, because there we have to send the header and body separately
    bool alreadySendHeader = false;

    static std::string getStatusMessage(int code);

    enum StatusCode {
        OK = 200,
        CREATED = 201,
        NO_CONTENT = 204,
        MOVED_PERMANENTLY = 301,
        BAD_REQUEST = 400,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        CONFLICT = 409,
        CONTENT_TOO_LARGE = 413,
        UNSUPPORTED_MEDIA_TYPE = 415,
        METHOD_NOT_ALLOWED = 405,
        INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED = 501,
    };

    HttpResponse(int statusCode = StatusCode::OK);

    static HttpResponse html(StatusCode statusCode, const std::string &bodyMessage = "");

    void setStatus(int code, const std::string &message = "");

    [[nodiscard]] int getStatus() const;

    void setHeader(const std::string &name, const std::string &value);

    void setBody(const std::string &body);

    // this is only used for sending files
    void enableChunkedEncoding(int bodyFd);

    [[nodiscard]] std::string toString() const;

    [[nodiscard]] std::string toHeaderString() const;

    [[nodiscard]] std::string getBody() const;

    [[nodiscard]] int getBodyFd() const;

    [[nodiscard]] bool isChunkedEncoding() const;

private:
    static void createNotFoundPage(std::stringstream &ss);
};


#endif //HTTPRESPONSE_H
