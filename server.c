#include <stdio.h>
#include <winsock2.h>
#include <process.h>
#include "utilities.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 4096


void handle_request(SOCKET client_socket);

// Thread function to handle client requests
unsigned __stdcall handle_client(void *client_socket_ptr)
{
    SOCKET client_socket = *(SOCKET *)client_socket_ptr;
    handle_request(client_socket); // Your existing request handler
    closesocket(client_socket);
    free(client_socket_ptr); // Free the allocated memory for the socket
    return 0;
}

int main()
{
    WSADATA wsa;
    SOCKET server_socket;
    struct sockaddr_in server_addr;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Winsock initialization failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET)
    {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(server_socket, 10) == SOCKET_ERROR)
    {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Server running on port %d...\n", PORT);

    while (1)
    {
        // Accept client connection
        SOCKET *client_socket = (SOCKET *)malloc(sizeof(SOCKET));
        int client_addr_len = sizeof(struct sockaddr_in);
        *client_socket = accept(server_socket, (struct sockaddr *)&server_addr, &client_addr_len);
        if (*client_socket == INVALID_SOCKET)
        {
            printf("Accept failed: %d\n", WSAGetLastError());
            free(client_socket);
            continue;
        }

        // Create a new thread for the client
        _beginthreadex(NULL, 0, handle_client, client_socket, 0, NULL);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}

void handle_request(SOCKET client_socket)
{
    char request_buffer[BUFFER_SIZE];
    int bytes_received;

    // Read client request
    bytes_received = recv(client_socket, request_buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0)
    {
        return;
    }

    log_http_message("Incoming HTTP Request", request_buffer, bytes_received);

    // Parse request line
    char *request_line = strtok(request_buffer, "\n");
    char *method = strtok(request_line, " ");
    char *path = strtok(NULL, " ");

    if (method == NULL || path == NULL)
    {
        send_http_response(client_socket, "400 Bad Request", "text/plain", "Invalid request");
        return;
    }

    // Only support GET requests
    if (strcmp(method, "GET") != 0)
    {
        send_http_response(client_socket, "405 Method Not Allowed", "text/plain", "Method not allowed");
        return;
    }

    // Default to "index.html" if root path ("/") is requested
    if (strcmp(path, "/") == 0)
    {
        path = "/index.html";
    }

    // Construct the local file path
    char file_path[256];
    snprintf(file_path, sizeof(file_path), ".%s", path);

    // Serve the file
    serve_file(client_socket, file_path);
}

