#include <stdio.h>
#include <winsock2.h>
#include <process.h>
#include "utilities.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 4096
#define MAX_THREADS 4 
#define MAX_QUEUE 100

//request queue
typedef struct
{
    SOCKET client_socket;
    struct sockaddr_in client_addr;
} Request;

// global variables for thread pool
Request request_queue[MAX_QUEUE];
int queue_start = 0;
int queue_end = 0;
int queue_size = 0;
CRITICAL_SECTION queue_lock;
HANDLE queue_semaphore;
HANDLE stop_event;
HANDLE worker_threads[MAX_THREADS];

void handle_request(SOCKET client_socket);

// create a queue for requests to handle them in a thread pool
unsigned __stdcall worker_thread(void *arg)
{
    while (WaitForSingleObject(stop_event, 0) != WAIT_OBJECT_0)
    {
        if (WaitForSingleObject(queue_semaphore, INFINITE) == WAIT_OBJECT_0)
        {
            Request request;

            // Get request from queue
            EnterCriticalSection(&queue_lock);
            if (queue_size > 0)
            {
                request = request_queue[queue_start];
                queue_start = (queue_start + 1) % MAX_QUEUE;
                queue_size--;
            }
            LeaveCriticalSection(&queue_lock);

            handle_request(request.client_socket);
            closesocket(request.client_socket);
        }
    }
    return 0;
}

unsigned __stdcall handle_client(void *client_socket_ptr)
{
    SOCKET client_socket = *(SOCKET *)client_socket_ptr;
    handle_request(client_socket);
    closesocket(client_socket);
    free(client_socket_ptr);
    return 0;
}

int main()
{
    WSADATA wsa;
    SOCKET server_socket;
    struct sockaddr_in server_addr;

    // check Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Winsock initialization failed: %d\n", WSAGetLastError());
        return 1;
    }

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

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR)
    {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Server running on port %d...\n", PORT);

    //Create a thread pool
    InitializeCriticalSection(&queue_lock);
    queue_semaphore = CreateSemaphore(NULL, 0, MAX_QUEUE, NULL);
    stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Create worker threads
    for (int i = 0; i < MAX_THREADS; i++)
    {
        worker_threads[i] = (HANDLE)_beginthreadex(NULL, 0, worker_thread, NULL, 0, NULL);
    }

    while (1)
    {
        // Accept client connection
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket == INVALID_SOCKET)
        {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        // Add request to queue
        EnterCriticalSection(&queue_lock);
        if (queue_size < MAX_QUEUE)
        {
            request_queue[queue_end].client_socket = client_socket;
            request_queue[queue_end].client_addr = client_addr;
            queue_end = (queue_end + 1) % MAX_QUEUE;
            queue_size++;
            ReleaseSemaphore(queue_semaphore, 1, NULL);
        }
        else
        {
            closesocket(client_socket);
        }
        LeaveCriticalSection(&queue_lock);
    }

    SetEvent(stop_event);
    WaitForMultipleObjects(MAX_THREADS, worker_threads, TRUE, INFINITE);
    for (int i = 0; i < MAX_THREADS; i++)
    {
        CloseHandle(worker_threads[i]);
    }
    DeleteCriticalSection(&queue_lock);
    CloseHandle(queue_semaphore);
    CloseHandle(stop_event);
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
    DWORD thread_id = GetCurrentThreadId();

    printf("[Thread %lu] Handling request...\n", thread_id);

    log_http_message("Incoming HTTP Request", request_buffer, bytes_received);

    //parse request line
    char *request_line = strtok(request_buffer, "\n");
    char *method = strtok(request_line, " ");
    char *path = strtok(NULL, " ");

    if (method == NULL || path == NULL)
    {
        send_http_response(client_socket, "400 Bad Request", "text/plain", "Invalid request");
        return;
    }
    //for get requests only
    if (strcmp(method, "GET") != 0)
    {
        send_http_response(client_socket, "405 Method Not Allowed", "text/plain", "Method not allowed");
        return;
    }

    // Default "index.html"
    if (strcmp(path, "/") == 0)
    {
        path = "/index.html";
    }

    // Construct the local file path
    char file_path[256];
    snprintf(file_path, sizeof(file_path), ".%s", path);

    serve_file(client_socket, file_path);

    printf("[Thread %lu] Request handled.\n", thread_id);
}
