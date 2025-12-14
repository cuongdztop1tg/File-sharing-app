#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common.h"

// Global array of session pointers
Session *sessions[MAX_CLIENTS] = {NULL};
// Mutex to protect the session array
pthread_mutex_t session_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Adds a new connection to the session list.
 */
void add_session(int sockfd, struct sockaddr_in addr) {
    pthread_mutex_lock(&session_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i] == NULL) {
            sessions[i] = (Session *)malloc(sizeof(Session));
            sessions[i]->socket_fd = sockfd;
            sessions[i]->user_id = -1; // Not logged in
            sessions[i]->is_logged_in = 0;
            strcpy(sessions[i]->username, "Guest");
            
            // Store IP Address
            inet_ntop(AF_INET, &(addr.sin_addr), sessions[i]->client_ip, INET_ADDRSTRLEN);
            
            break;
        }
    }
    pthread_mutex_unlock(&session_lock);
}

/**
 * @brief Removes a session when client disconnects.
 */
void remove_session(int sockfd) {
    pthread_mutex_lock(&session_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i] != NULL && sessions[i]->socket_fd == sockfd) {
            free(sessions[i]);
            sessions[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&session_lock);
}

/**
 * @brief Finds a session by socket ID.
 */
Session *find_session(int sockfd) {
    pthread_mutex_lock(&session_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i] != NULL && sessions[i]->socket_fd == sockfd) {
            pthread_mutex_unlock(&session_lock);
            return sessions[i];
        }
    }
    pthread_mutex_unlock(&session_lock);
    return NULL;
}