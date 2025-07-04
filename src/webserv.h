/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eebert <eebert@student.42heilbronn.de>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/06 13:49:46 by eebert            #+#    #+#             */
/*   Updated: 2025/06/17 22:02:00 by eebert           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_H
#define WEBSERV_H

#define READ_FILE_TIMEOUT 1000
#define CGI_TIMEOUT 1000
#define SERVER_NAME "webserv"
#define TEMP_DIR_NAME ".tmp"
#define SESSION_SAVE_FILE ".sessions.bin"

#if defined(__APPLE__)
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#endif

#endif