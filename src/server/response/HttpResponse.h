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

    static std::string getStatusMessage(int code) ;

public:
    enum StatusCode {
        OK = 200,
        CREATED = 201,
        NO_CONTENT = 204,
        MOVED_PERMANENTLY = 301,
        BAD_REQUEST = 400,
        FORBIDDEN = 403,
        CONFLICT = 409,
        NOT_FOUND = 404,
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

    void enableChunkedEncoding();

    void addChunk(const std::string &chunk);

    [[nodiscard]] std::string toString() const;

    [[nodiscard]] std::string getBody() const;

private:
    static void createNotFoundPage(std::stringstream &ss);
};


#endif //HTTPRESPONSE_H
