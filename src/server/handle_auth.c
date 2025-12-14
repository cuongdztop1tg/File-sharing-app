#include <stdio.h>
#include <string.h>
#include "common.h"
#include "protocol.h"
#include "network.h"

// Forward declarations (should be in headers)
int db_check_login(const char *username, const char *password);
int db_register_user(const char *username, const char *password);
Session *find_session(int sockfd);
void log_activity(const char *msg);

void handle_login(int sockfd, char *payload) {
    char user[50], pass[50];
    
    // Parse payload: "username password"
    if (sscanf(payload, "%s %s", user, pass) < 2) {
        char *msg = "Missing username or password";
        send_packet(sockfd, MSG_ERROR, msg, strlen(msg));
        return;
    }

    int user_id = db_check_login(user, pass);
    
    if (user_id != -1) {
        // 1. Update Session
        Session *sess = find_session(sockfd);
        if (sess) {
            sess->user_id = user_id;
            sess->is_logged_in = 1;
            strcpy(sess->username, user);
        }

        // 2. Send Success Response
        char msg[100];
        sprintf(msg, "Login successful. Welcome %s (ID: %d)", user, user_id);
        send_packet(sockfd, MSG_SUCCESS, msg, strlen(msg));
        
        // 3. Log
        char log_msg[200];
        sprintf(log_msg, "User '%s' (ID %d) logged in from %s", 
                user, user_id, sess ? sess->client_ip : "Unknown");
        log_activity(log_msg);

    } else {
        char *msg = "Invalid username or password";
        send_packet(sockfd, MSG_ERROR, msg, strlen(msg));
        
        log_activity("Failed login attempt.");
    }
}

void handle_register(int sockfd, char *payload) {
    char user[50], pass[50];
    
    if (sscanf(payload, "%s %s", user, pass) < 2) {
        send_packet(sockfd, MSG_ERROR, "Invalid format", 14);
        return;
    }

    int new_id = db_register_user(user, pass);
    
    if (new_id != -1) {
        char msg[100];
        sprintf(msg, "Registration successful. Please login. ID: %d", new_id);
        send_packet(sockfd, MSG_SUCCESS, msg, strlen(msg));
        
        char log_msg[200];
        sprintf(log_msg, "New user registered: %s (ID %d)", user, new_id);
        log_activity(log_msg);
    } else {
        send_packet(sockfd, MSG_ERROR, "Username already exists", 23);
    }
}