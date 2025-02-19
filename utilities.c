#include "utilities.h"
#include <stdio.h>
#include <string.h>
#include <io.h>

// Send HTTP response headers and content
void send_http_response(SOCKET client_socket, const char *status, const char *content_type, const char *content)
{
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n\r\n"
             "%s",
             status, content_type, strlen(content), content);

    log_http_message("Outgoing HTTP Response", response, strlen(response));

    send(client_socket, response, strlen(response), 0);
}

// Serve a file if it exists
void serve_file(SOCKET client_socket, const char *file_path)
{
    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        FILE *error_file = fopen("404.html", "rb");
        if (error_file)
        {
            // Get 404 file size
            fseek(error_file, 0, SEEK_END);
            long error_size = ftell(error_file);
            fseek(error_file, 0, SEEK_SET);

            // read 404 file content
            char *error_content = (char *)malloc(error_size + 1);
            fread(error_content, 1, error_size, error_file);
            fclose(error_file);

            // Send 404 response 
            char headers[BUFFER_SIZE];
            snprintf(headers, sizeof(headers),
                     "HTTP/1.1 404 Not Found\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length: %ld\r\n\r\n",
                     error_size);

            log_http_message("Outgoing HTTP Response", headers, strlen(headers));

            send(client_socket, headers, strlen(headers), 0);
            send(client_socket, error_content, error_size, 0);

            free(error_content);
        }
        else
        {
            char error_response[BUFFER_SIZE];
            snprintf(error_response, sizeof(error_response),
                     "HTTP/1.1 404 Not Found\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: 14\r\n\r\n"
                     "File not found");

            log_http_message("Outgoing HTTP Response", error_response, strlen(error_response));

            send(client_socket, error_response, strlen(error_response), 0);
        }
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read file content
    char *file_content = (char *)malloc(file_size + 1);
    fread(file_content, 1, file_size, file);
    fclose(file);

    // Determine MIME type based on file extension
    const char *file_ext = strrchr(file_path, '.');
    const char *mime_type = get_mime_type(file_ext ? file_ext + 1 : "");

    // Send HTTP response
    char headers[BUFFER_SIZE];
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n\r\n",
             mime_type, file_size);
    send(client_socket, headers, strlen(headers), 0);
    send(client_socket, file_content, file_size, 0);

    log_http_message("Outgoing HTTP Response", headers, strlen(headers));

    free(file_content);
}

// get MIME type based on file extension
const char *get_mime_type(const char *file_ext)
{
    if (strcmp(file_ext, "html") == 0)
        return "text/html";
    if (strcmp(file_ext, "css") == 0)
        return "text/css";
    if (strcmp(file_ext, "js") == 0)
        return "application/javascript";
    if (strcmp(file_ext, "png") == 0)
        return "image/png";
    if (strcmp(file_ext, "jpg") == 0 || strcmp(file_ext, "jpeg") == 0)
        return "image/jpeg";
    if (strcmp(file_ext, "gif") == 0)
        return "image/gif";
    if (strcmp(file_ext, "json") == 0)
        return "application/json";
    return "text/plain";
}

// log HTTP message to console or use wireshark
void log_http_message(const char *prefix, const char *message, int length)
{
    printf("\n%s:\n", prefix);
    printf("----------------------------------------\n");
    char *first_line = strdup(message);
    char *end = strchr(first_line, '\r');
    if (end)
        *end = '\0';
    printf("%s\n", first_line);
    printf("Message length: %d bytes\n", length);
    printf("----------------------------------------\n");
    free(first_line);
}