#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include "utilities.h"
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 0722
#define BUFFER_SIZE 4096

void handle_request(SOCKET client_socket)
{
    DWORD thread_id = GetCurrentThreadId();
    printf("\n[Thread %lu] Handling new request...\n", thread_id);

    char request_buffer[BUFFER_SIZE];
    int bytes_received;

    bytes_received = recv(client_socket, request_buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0)
    {
        printf("[Thread %lu] Connection closed by client.\n", thread_id);
        return;
    }

    log_http_message("Incoming HTTP Request", request_buffer, bytes_received);
    
    char *request_line = strtok(request_buffer, "\n");
    char *method = strtok(request_line, " ");
    char *path = strtok(NULL, " ");

    if (method == NULL || path == NULL)
    {
        send_http_response(client_socket, "400 Bad Request", "text/plain", "Invalid request");
        printf("[Thread %lu] Bad request handled.\n", thread_id);
        return;
    }

    // Only support GET requests
    if (strcmp(method, "GET") != 0)
    {
        send_http_response(client_socket, "405 Method Not Allowed", "text/plain", "Method not allowed");
        printf("[Thread %lu] Method not allowed handled.\n", thread_id);
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
    printf("[Thread %lu] Request handled.\n", thread_id);
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
    if (listen(server_socket, 1) == SOCKET_ERROR)
    {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Simple server running on port %d...\n", PORT);
    printf("Main server thread ID: %lu\n", GetCurrentThreadId());

    while (1)
    {
        // Accept client connection
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket == INVALID_SOCKET)
        {
            printf("[Thread %lu] Accept failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
            continue;
        }

        // Log client connection with thread ID
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("[Thread %lu] New connection from %s:%d\n",
               GetCurrentThreadId(),
               client_ip,
               ntohs(client_addr.sin_port));

        // Handle the request directly (blocking)
        handle_request(client_socket);
        closesocket(client_socket);
    }
    // Cleanup
    closesocket(server_socket);
    WSACleanup();
    return 0;
}