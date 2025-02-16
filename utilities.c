// utilities.c
#include "utilities.h"
#include <stdio.h>
#include <string.h>
#include <io.h>  // For file existence checks on Windows

// Send HTTP response headers and content
void send_http_response(SOCKET client_socket, const char* status, const char* content_type, const char* content) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n\r\n"
        "%s",
        status, content_type, strlen(content), content);
    send(client_socket, response, strlen(response), 0);
}

// Serve a file if it exists
void serve_file(SOCKET client_socket, const char* file_path) {
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        send_http_response(client_socket, "404 Not Found", "text/plain", "File not found");
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read file content
    char* file_content = (char*)malloc(file_size + 1);
    fread(file_content, 1, file_size, file);
    fclose(file);

    // Determine MIME type based on file extension
    const char* file_ext = strrchr(file_path, '.');
    const char* mime_type = get_mime_type(file_ext ? file_ext + 1 : "");

    // Send HTTP response with file content
    char headers[BUFFER_SIZE];
    snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n\r\n",
        mime_type, file_size);
    send(client_socket, headers, strlen(headers), 0);
    send(client_socket, file_content, file_size, 0);

    free(file_content);
}

// Map file extensions to MIME types
const char* get_mime_type(const char* file_ext) {
    if (strcmp(file_ext, "html") == 0) return "text/html";
    if (strcmp(file_ext, "css") == 0) return "text/css";
    if (strcmp(file_ext, "js") == 0) return "application/javascript";
    if (strcmp(file_ext, "png") == 0) return "image/png";
    if (strcmp(file_ext, "jpg") == 0) return "image/jpeg";
    return "text/plain";  // Default
}