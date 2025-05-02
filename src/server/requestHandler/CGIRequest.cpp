#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <__filesystem/operations.h>

#include "common/Logger.h"
#include "RequestHandler.h"

std::optional<HttpResponse> RequestHandler::handleCgi() {
    if (!std::filesystem::exists(cgiPath) || !std::filesystem::is_regular_file(cgiPath) ||
        access(cgiPath.c_str(), X_OK) != 0) {
        Logger::log(LogLevel::ERROR, "CGI interpreter not found or not executable: " + cgiPath);
        return std::nullopt;
    }

    if (!std::filesystem::exists(routePath) || !std::filesystem::is_regular_file(routePath) ||
        access(routePath.c_str(), R_OK) != 0) {
        Logger::log(LogLevel::ERROR, "Target CGI script not found or not readable: " + routePath);
        return std::nullopt;
    }

    int input_pipe[2];  // Parent -> Child
    int output_pipe[2]; // Child -> Parent
    
    if (pipe(input_pipe) < 0 || pipe(output_pipe) < 0) {
        Logger::log(LogLevel::ERROR, "Failed to create pipes for CGI");
        return Response::customResponse(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR, 
                                       "CGI Error: Could not create pipes");
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(input_pipe[0]);
        close(input_pipe[1]);
        close(output_pipe[0]);
        close(output_pipe[1]);
        Logger::log(LogLevel::ERROR, "Failed to fork process for CGI");
        return Response::customResponse(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR, 
                                       "CGI Error: Could not fork process");
    }

    if (pid == 0) {
        dup2(input_pipe[0], STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);
        
        close(input_pipe[1]);
        close(output_pipe[0]);
        
        // TODO implement query string
        setenv("QUERY_STRING", "", 1);
        setenv("REQUEST_METHOD", request.getMethodString().c_str(), 1);
        setenv("CONTENT_TYPE", request.getHeader("content-type").c_str(), 1);
        setenv("CONTENT_LENGTH", std::to_string(request.body.size()).c_str(), 1);
        setenv("SCRIPT_NAME", routePath.c_str(), 1);
        setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
        setenv("SERVER_SOFTWARE", "Webserv/1.0", 1);
        
        execl(cgiPath.c_str(), cgiPath.c_str(), routePath.c_str(), nullptr);
        
        Logger::log(LogLevel::ERROR, "Failed to execute CGI script");
        exit(EXIT_FAILURE);
    }

    close(input_pipe[0]);
    close(output_pipe[1]);

    fcntl(input_pipe[1], F_SETFL, O_NONBLOCK);
    fcntl(output_pipe[0], F_SETFL, O_NONBLOCK);

    pollfd write_fd;
    write_fd.fd = input_pipe[1];
    write_fd.events = POLLOUT;
    
    size_t bytesWritten = 0;
    while (bytesWritten < request.body.size()) {
        int poll_result = poll(&write_fd, 1, 5000);
        
        if (poll_result < 0) {
            Logger::log(LogLevel::ERROR, "Poll error while writing to CGI");
            break;
        }
        if (poll_result == 0) {
            Logger::log(LogLevel::ERROR, "Timeout while writing to CGI");
            break;
        }
        if (write_fd.revents & POLLOUT) {
            ssize_t written = write(input_pipe[1], request.body.c_str() + bytesWritten, 
                                    request.body.size() - bytesWritten);
            if (written <= 0) break;
            bytesWritten += written;
        }
    }
    
    close(input_pipe[1]);

    pollfd read_fd;
    read_fd.fd = output_pipe[0];
    read_fd.events = POLLIN;
    
    std::string cgiOutput;
    char buffer[4096];
    bool keepReading = true;
    
    while (keepReading) {
        int poll_result = poll(&read_fd, 1, 10000);
        
        if (poll_result < 0) {
            Logger::log(LogLevel::ERROR, "Poll error while reading from CGI");
            break;
        }
        if (poll_result == 0) {
            Logger::log(LogLevel::ERROR, "Timeout while reading from CGI");
            break;
        }
        if (read_fd.revents & POLLIN) {
            ssize_t bytesRead = read(output_pipe[0], buffer, sizeof(buffer) - 1);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                cgiOutput.append(buffer, bytesRead);
            } else {
                keepReading = false;
            }
        } else if (read_fd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            keepReading = false;
        }
    }
    
    close(output_pipe[0]);
    
    int status;
    pid_t result = waitpid(pid, &status, WNOHANG);
    
    if (result == 0) {
        kill(pid, SIGTERM);
        usleep(100000);
        waitpid(pid, &status, WNOHANG);
    }
    
    HttpResponse response;
    
    size_t headerEnd = cgiOutput.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        response = HttpResponse(HttpResponse::StatusCode::OK);
        response.setHeader("Content-Type", "text/html");
        response.setBody(cgiOutput);
    } else {
        std::string headers = cgiOutput.substr(0, headerEnd);
        std::string body = cgiOutput.substr(headerEnd + 4);
        
        response = HttpResponse(HttpResponse::StatusCode::OK);
        
        std::istringstream headerStream(headers);
        std::string line;
        while (std::getline(headerStream, line)) {
            if (line.empty() || line == "\r") continue;
            
            if (!line.empty() && line[line.size()-1] == '\r') {
                line.erase(line.size()-1);
            }
            
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string name = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                
                while (!value.empty() && value[0] == ' ') {
                    value.erase(0, 1);
                }
                
                if (name == "Status") {
                    int statusCode = std::stoi(value.substr(0, 3));
                    response.setStatus(statusCode);
                } else {
                    response.setHeader(name, value);
                }
            }
        }
        
        response.setBody(body);
    }
    return response;
}