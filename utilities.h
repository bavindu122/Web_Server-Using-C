#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <winsock2.h>

#define BUFFER_SIZE 4096

void send_http_response(SOCKET client_socket, const char *status, const char *content_type, const char *content);
void serve_file(SOCKET client_socket, const char *file_path);
const char *get_mime_type(const char *file_ext);
void log_http_message(const char *prefix, const char *message, int length);

#endif