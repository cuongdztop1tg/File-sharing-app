#include "protocol.h"
#include "network.h"
#include <stdio.h>

// External functions (Logic Handlers)
void handle_login(int sockfd, char *payload);
void handle_register(int sockfd, char *payload);
// void handle_create_group(int sockfd, char *payload); // Future implementation

void process_client_request(int sockfd, int msg_type, char *payload) {
    switch (msg_type) {
        case MSG_LOGIN:
            handle_login(sockfd, payload);
            break;
            
        case MSG_REGISTER:
            handle_register(sockfd, payload);
            break;
            
        case MSG_LOGOUT:
            // TODO: Handle logout logic (update session)
            break;
            
        // Add Group/File cases here...
        
        default:
            printf("Unknown message type: %d\n", msg_type);
            send_packet(sockfd, MSG_ERROR, "Unknown command", 15);
            break;
    }
}