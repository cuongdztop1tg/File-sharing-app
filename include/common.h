#ifndef COMMON_H
#define COMMON_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

// --- CONFIGURATION CONSTANTS ---
#define SERVER_PORT 3636
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 100

// --- FILE PATHS ---
#define DATA_DIR "./data"
#define USER_DB_FILE "./data/users.txt"
#define LOG_FILE "./data/server.log"

// --- DATA STRUCTURES ---

// User model for Database
typedef struct {
    int id;
    char username[50];
    char password[50];
} User;

// Session model for Runtime
typedef struct {
    int socket_fd;          // Socket descriptor
    int user_id;            // User ID
    char username[50];      // Username
    char client_ip[INET_ADDRSTRLEN]; // Client IP Address
    int is_logged_in;       // 0: No, 1: Yes
} Session;

#endif