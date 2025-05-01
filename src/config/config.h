/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eebert <eebert@student.42heilbronn.de>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/01 13:19:13 by eebert            #+#    #+#             */
/*   Updated: 2025/05/01 19:34:06 by eebert           ###   ########.fr       */
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

typedef enum {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    OPTIONS
} HttpMethod;

typedef struct {
    std::string path;
    std::vector<HttpMethod> allowedMethods;
    std::string location;
    std::string root; // Override server root
    std::string cgiBin;
    bool autoindex; // Directory listing
    std::string index; // Override server index
    std::string alias; // Path substitution
    int client_max_body_size; // In bytes, override server setting
    std::map<int, std::string> error_pages; // Status code to page path
    std::vector<std::string> try_files; // Ordered list of files to try
    bool deny_all; // Access control
    std::map<std::string, std::string> fastcgi_params; // FastCGI parameters
    std::map<std::string, std::string> return_directive; // Redirects
} RouteConfig;

typedef struct {
    int port;
    std::string host;
    std::vector<std::string> server_names;
    std::string root;
    std::vector<RouteConfig> routes;
    std::string index;
    std::map<int, std::string> error_pages; // Status code to error page mapping

    // Connection settings
    int timeout;
    int client_max_body_size; // In bytes
    int client_header_buffer_size; // In bytes
    int keepalive_timeout; // In seconds
    int keepalive_requests; // Max requests per connection
} ServerConfig;

#endif //CONFIG_H
