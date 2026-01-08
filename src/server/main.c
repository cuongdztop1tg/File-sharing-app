#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "common.h"
#include "network.h"

// Declare external functions
void add_session(int sockfd, struct sockaddr_in addr);
void remove_session(int sockfd);
void process_client_request(int sockfd, int msg_type, char *payload);
void log_activity(const char *msg);

// Thread function
void *client_handler(void *arg) {
    int sock = *(int*)arg;
    free(arg);

    int msg_type;
    char buffer[BUFFER_SIZE];
    int payload_len;

    // Loop to receive packets
    while ((payload_len = recv_packet(sock, &msg_type, buffer)) >= 0) {
        process_client_request(sock, msg_type, buffer);
    }

    // Client disconnected
    remove_session(sock); // <--- REMOVE SESSION
    
    char log_msg[50];
    sprintf(log_msg, "Client (Socket %d) disconnected.", sock);
    log_activity(log_msg);
    
    close(sock);
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Fail to create server socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Fail to bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 10) < 0) {
        perror("Fail to listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    
    printf("Server started. Listening on port %d...\n", SERVER_PORT);
    log_activity("Server started.");

    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);

        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // --- ADD SESSION HERE ---
        add_session(client_sock, client_addr);
        
        char log_msg[100];
        sprintf(log_msg, "New connection from %s", inet_ntoa(client_addr.sin_addr));
        log_activity(log_msg);

        // Create Thread
        pthread_t tid;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        if (pthread_create(&tid, NULL, client_handler, (void*)new_sock) < 0) {
            perror("Thread creation failed");
            free(new_sock);
            close(client_sock);
            continue;
        }
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}