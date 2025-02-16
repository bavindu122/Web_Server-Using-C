// server.c
#include <stdio.h>
#include <winsock2.h>
#include "utilities.h"  // For your helper functions

#pragma comment(lib, "ws2_32.lib")  // Link Winsock library

#define PORT 8080
#define BUFFER_SIZE 4096

void handle_request(SOCKET client_socket);

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock initialization failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(server_socket, 10) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Server running on port %d...\n", PORT);

    while (1) {
        // Accept client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        // Handle client request
        handle_request(client_socket);  // Your request handler (defined below)

        closesocket(client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}

// server.c (continued)
void handle_request(SOCKET client_socket) {
    char request_buffer[BUFFER_SIZE];
    char response_buffer[BUFFER_SIZE];
    int bytes_received;

    // Read client request
    bytes_received = recv(client_socket, request_buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        return;  // No data or error
    }

    // Extract the requested path (e.g., "GET /index.html HTTP/1.1")
    char* request_line = strtok(request_buffer, "\n");
    char* method = strtok(request_line, " ");
    char* path = strtok(NULL, " ");

    if (method == NULL || path == NULL) {
        send_http_response(client_socket, "400 Bad Request", "text/plain", "Invalid request");
        return;
    }

    // Default to "index.html" if root path ("/") is requested
    if (strcmp(path, "/") == 0) {
        path = "/index.html";
    }

    // Construct the local file path (adjust based on your folder structure)
    char file_path[256];
    snprintf(file_path, sizeof(file_path), ".%s", path);  // Example: "./index.html"

    // Serve the file
    serve_file(client_socket, file_path);
}