#include "RequestHandler.h"

#include <sys/stat.h>
#include <fstream>
#include <regex>
#include <filesystem>

HttpResponse RequestHandler::handlePost() const {
    if (!std::filesystem::exists(routePath)) {
        return Response::customResponse(HttpResponse::StatusCode::NOT_FOUND,
                                        "404 Not Found: Target directory does not exist");
    }

    if (isFile) {
        return Response::customResponse(HttpResponse::StatusCode::BAD_REQUEST,
                                        "400 Bad Request: Target must be a directory for file uploads");
    }

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
    std::string endBoundary = boundary + "--";

    size_t pos = 0;
    int filesUploaded = 0;
    std::vector<std::string> uploadedFiles;

    while (true) {
        size_t partStart = request.body.find(boundary, pos);
        if (partStart == std::string::npos)
            break;

        if (request.body.substr(partStart, endBoundary.length()) == endBoundary)
            break;

        partStart += boundary.length();

        if (request.body.substr(partStart, 2) == "\r\n")
            partStart += 2;
        else
            break;

        size_t headersEnd = request.body.find("\r\n\r\n", partStart);
        if (headersEnd == std::string::npos)
            break;

        std::string headers = request.body.substr(partStart, headersEnd - partStart);

        std::regex filenameRegex("filename\\s*=\\s*\"([^\"]*)\"");
        if (!std::regex_search(headers, match, filenameRegex)) {
            return Response::customResponse(HttpResponse::StatusCode::BAD_REQUEST,
                                            "400 Bad Request: Missing filename");
        }

        std::string filename = match[1].str();
        if (filename.empty()) {
            return Response::customResponse(HttpResponse::StatusCode::BAD_REQUEST,
                                            "400 Bad Request: Missing filename");
        }

        std::filesystem::path fullPath = std::filesystem::path(routePath) / filename;

        headersEnd += 4;

        size_t partEnd = request.body.find(boundary, headersEnd);
        if (partEnd == std::string::npos)
            partEnd = request.body.size();

        std::string fileContent = request.body.substr(headersEnd, partEnd - headersEnd - 2);

        std::ofstream file(fullPath, std::ios::binary);
        if (!file)
            return Response::customResponse(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                            "500 Internal Server Error: Could not write file");

        file.write(fileContent.c_str(), fileContent.size());
        file.close();

        uploadedFiles.push_back(filename);
        filesUploaded++;

        pos = partEnd;
    }

    if (filesUploaded == 0) {
        return Response::customResponse(HttpResponse::StatusCode::BAD_REQUEST,
                                        "400 Bad Request: No files found in request");
    }

    HttpResponse response(HttpResponse::StatusCode::CREATED);
    response.setBody("201 Created: " + std::to_string(filesUploaded) + " file(s) uploaded successfully");
    response.setHeader("Content-Type", "text/plain");
    return response;
}
