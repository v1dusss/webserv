#include "RequestHandler.h"

#include <sys/stat.h>
#include <fstream>
#include <regex>
#include <filesystem>

HttpResponse RequestHandler::handlePost() const {
    if (!std::filesystem::exists(routePath)) {
        return HttpResponse::html(HttpResponse::StatusCode::NOT_FOUND,
                                  "Target directory does not exist");
    }

    if (isFile) {
        return HttpResponse::html(HttpResponse::StatusCode::BAD_REQUEST,
                                  "Target must be a directory for file uploads");
    }

    if (request.body.empty())
        return HttpResponse::html(HttpResponse::StatusCode::NO_CONTENT,
                                  "Empty request body");

    std::string contentType = request.getHeader("Content-Type");

    if (contentType.find("multipart/form-data") != std::string::npos)
        return handlePostMultipart(contentType);

    if (contentType.find("test/file") != std::string::npos)
        return handlePostTestFile();

    return HttpResponse::html(HttpResponse::StatusCode::UNSUPPORTED_MEDIA_TYPE,
                              "Invalid Content-Type");
}

HttpResponse RequestHandler::handlePostTestFile() const {
// TODO: create a config for the maximum size of the request body for uploads
    if (request.body.size() > 100)
        return HttpResponse::html(HttpResponse::StatusCode::CONTENT_TOO_LARGE);

    std::string filename = "test_file_" + std::to_string(std::time(nullptr)) + ".txt";
    std::filesystem::path fullPath = std::filesystem::path(routePath) / filename;
    std::ofstream file(fullPath, std::ios::binary);
    if (!file) {
        return HttpResponse::html(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                  "Could not write file");
    }
    file.write(request.body.c_str(), request.body.size());
    file.close();

    return HttpResponse::html(HttpResponse::StatusCode::CREATED,
                              "201 Created: " + filename + " file uploaded successfully");
}

HttpResponse RequestHandler::handlePostMultipart(const std::string &contentType) const {
    std::smatch match;

    std::regex boundaryRegex("boundary\\s*=\\s*([^;]+)");
    if (!std::regex_search(contentType, match, boundaryRegex)) {
        return HttpResponse::html(HttpResponse::StatusCode::BAD_REQUEST,
                                  "Missing boundary");
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
            return HttpResponse::html(HttpResponse::StatusCode::BAD_REQUEST,
                                      "Missing filename");
        }

        std::string filename = match[1].str();
        if (filename.empty()) {
            return HttpResponse::html(HttpResponse::StatusCode::BAD_REQUEST,
                                      "Missing filename");
        }

        std::filesystem::path fullPath = std::filesystem::path(routePath) / filename;

        headersEnd += 4;

        size_t partEnd = request.body.find(boundary, headersEnd);
        if (partEnd == std::string::npos)
            partEnd = request.body.size();

        std::string fileContent = request.body.substr(headersEnd, partEnd - headersEnd - 2);

        std::ofstream file(fullPath, std::ios::binary);
        if (!file)
            return HttpResponse::html(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                      "Could not write file");

        file.write(fileContent.c_str(), fileContent.size());
        file.close();

        uploadedFiles.push_back(filename);
        filesUploaded++;

        pos = partEnd;
    }

    if (filesUploaded == 0) {
        return HttpResponse::html(HttpResponse::StatusCode::BAD_REQUEST,
                                  "No files found in request");
    }

    return HttpResponse::html(HttpResponse::StatusCode::CREATED,
                              "201 Created: " + std::to_string(filesUploaded) + " file(s) uploaded successfully");
}
