#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>
#include <fcntl.h>
#include <csignal>
#include <filesystem>
#include <server/FdHandler.h>

#include "common/Logger.h"
#include "RequestHandler.h"
#include "server/ClientConnection.h"

bool RequestHandler::validateCgiEnvironment() const {
    const std::string filePath = getFilePath();
    if (!std::filesystem::exists(cgiPath) || !std::filesystem::is_regular_file(cgiPath) ||
        access(cgiPath.c_str(), X_OK) != 0) {
        Logger::log(LogLevel::ERROR, "CGI interpreter not found or not executable: " + cgiPath);
        return false;
    }

    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath) ||
        access(filePath.c_str(), R_OK) != 0) {
        Logger::log(LogLevel::ERROR, "Target CGI script not found or not readable: " + filePath);
        return false;
    }
    return true;
}

static bool setupPipes(int input_pipe[2], int output_pipe[2]) {
    if (pipe(input_pipe) < 0 || pipe(output_pipe) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to create pipes for CGI");
        return false;
    }
    return true;
}

void RequestHandler::configureCgiChildProcess(int input_pipe[2], int output_pipe[2]) const {
    const std::string filePath = getFilePath();
    dup2(input_pipe[0], STDIN_FILENO);
    dup2(output_pipe[1], STDOUT_FILENO);

    close(input_pipe[1]);
    close(output_pipe[0]);

    std::unordered_map<std::string, std::string> env;


    for (const auto &header: request.headers) {
        std::string name = header.first;
        std::string value = header.second;

        std::string envName = "HTTP_";
        for (char c: name) {
            if (c == '-') {
                envName += '_';
            } else {
                envName += std::toupper(c);
            }
        }

        env[envName] = value;
    }

    env["QUERY_STRING"] = request.getQueryString();
    env["REQUEST_METHOD"] = request.getMethodString();
    env["CONTENT_TYPE"] = request.getHeader("Content-Type");
    env["CONTENT_LENGTH"] = std::to_string(request.body.size());
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["SERVER_SOFTWARE"] = "Webserv/1.0";
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_NAME"] = serverConfig.host;
    env["SERVER_PORT"] = std::to_string(serverConfig.port);
    env["PATH_INFO"] = request.getPath();


    std::vector<char *> envp;
    for (const auto &[fst, snd]: env) {
        std::string envVar = fst + "=" + snd;
        envp.push_back(strdup(envVar.data()));
    }
    envp.push_back(nullptr);
    char *const argv[] = {const_cast<char *>(cgiPath.c_str()), const_cast<char *>(filePath.c_str()), nullptr};
    execve(cgiPath.c_str(), argv, envp.data());

    Logger::log(LogLevel::ERROR, "Failed to execute CGI script");
    exit(EXIT_FAILURE);
}

static void cleanupCgiProcess(const pid_t pid) {
    int status;
    const pid_t result = waitpid(pid, &status, WNOHANG);

    if (result == 0) {
        kill(pid, SIGTERM);
        usleep(100000);
        waitpid(pid, &status, WNOHANG);
    }
}

static HttpResponse parseCgiOutput(const std::string &cgiOutput) {
    HttpResponse response;

    const size_t headerEnd = cgiOutput.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        response = HttpResponse(HttpResponse::StatusCode::OK);
        response.setHeader("Content-Type", "text/html");
        response.setBody(cgiOutput);
        return response;
    }

    const std::string headers = cgiOutput.substr(0, headerEnd);
    const std::string body = cgiOutput.substr(headerEnd + 4);

    response = HttpResponse(HttpResponse::StatusCode::OK);

    std::istringstream headerStream(headers);
    std::string line;
    while (std::getline(headerStream, line)) {
        if (line.empty() || line == "\r") continue;

        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }

        const size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);

            while (!value.empty() && value[0] == ' ') {
                value.erase(0, 1);
            }

            if (name == "Status") {
                const int statusCode = std::stoi(value.substr(0, 3));
                response.setStatus(statusCode);
            } else {
                response.setHeader(name, value);
            }
        }
    }

    response.setBody(body);
    return response;
}

std::optional<HttpResponse> RequestHandler::handleCgi() {
    int input_pipe[2]; // Parent -> Child
    int output_pipe[2]; // Child -> Parent

    if (!setupPipes(input_pipe, output_pipe)) {
        return HttpResponse::html(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                  "CGI Error: Could not create pipes");
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(input_pipe[0]);
        close(input_pipe[1]);
        close(output_pipe[0]);
        close(output_pipe[1]);
        Logger::log(LogLevel::ERROR, "Failed to fork process for CGI");
        return HttpResponse::html(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                  "CGI Error: Could not fork process");
    }


    if (pid == 0) {
        configureCgiChildProcess(input_pipe, output_pipe);
    }

    close(input_pipe[0]);
    close(output_pipe[1]);

    const int pipeFd = input_pipe[1];
    FdHandler::addFd(pipeFd, POLLOUT, [ this](int fd, short events) {
        (void) fd;
        (void) events;
        const ssize_t bytesToWrite = std::min(static_cast<ssize_t>(1024),
                                              static_cast<ssize_t>(request.body.size() - bytesWrittenToCgi));


        if (bytesToWrite == 0) {
            close(fd);
            return true;
        }

        const ssize_t written = write(fd, request.body.c_str() + bytesWrittenToCgi,
                                      bytesToWrite);
        bytesWrittenToCgi += written;
        return false;
    });
    FdHandler::addFd(output_pipe[0], POLLIN | POLLHUP, [pid, this](int fd, short events) {
        (void) events;

        ssize_t bytesRead = 0;
        char buffer[4096];
        bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            cgiOutputBuffer.append(buffer, bytesRead);
            return false;
        }

        if (bytesRead == 0 || events & (POLLHUP)) {
            close(fd);
            cleanupCgiProcess(pid);
            const HttpResponse response = parseCgiOutput(cgiOutputBuffer);
            client->setResponse(response);
            return true;
        }
        return false;
    });
    return std::nullopt;
}
