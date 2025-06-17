/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eebert <eebert@student.42heilbronn.de>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/01 13:19:13 by eebert            #+#    #+#             */
/*   Updated: 2025/06/16 19:05:25 by eebert           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

//
// Created by Emil Ebert on 01.05.25.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

class HttpRequest;
class HttpResponse;

typedef enum {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
} HttpMethod;

enum class LocationType {
    EXACT,
    PREFIX,
    REGEX,
    REGEX_IGNORE_CASE,
};

typedef struct {
    std::string location;
    LocationType type;
    std::vector<HttpMethod> allowedMethods;
    std::string root; // Override server root
    bool autoindex; // Directory listing
    std::string index; // Override server index
    std::string alias; // Path substitution
    std::map<int, std::string> error_pages; // Status code to page path
    bool deny_all; // Access control
    std::map<std::string, std::string> cgi_params;
    std::pair<int, std::string> return_directive;
    std::function<HttpResponse(const std::shared_ptr<HttpRequest> &request)> internalHandler;
// Redirects
} RouteConfig;

typedef struct  {
    size_t client_header_timeout; // In seconds
    size_t client_max_header_size; // In bytes the max size of a single header
    size_t client_max_header_count; // Max number of headers
} ClientHeaderConfig;

typedef struct {
    int port;
    std::string host;
    std::vector<std::string> server_names;
    std::string root;
    std::vector<RouteConfig> routes;
    std::string index;
    std::map<int, std::string> error_pages; // Status code to error page mapping

    ClientHeaderConfig headerConfig;
    // Connection settings
    size_t client_body_timeout; // In seconds
    size_t client_max_body_size; // In bytes
    size_t send_body_buffer_size;
    size_t body_buffer_size;
    size_t keepalive_timeout; // In seconds
    size_t cgi_timeout; // In seconds
    size_t keepalive_requests; // Max requests per connection
} ServerConfig;

typedef struct {
    ClientHeaderConfig headerConfig;
    size_t max_request_line_size;
}HttpConfig;

#endif //CONFIG_H
