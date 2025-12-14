#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "common.h"
#include "network.h"
#include "protocol.h"
// #include "client_ui.h" // Uncomment if you have UI functions separated

// --- FUNCTION PROTOTYPES ---
void handle_server_message(int sockfd);
void handle_user_input(int sockfd);
void print_menu();

// --- MAIN LOOP ---

/**
 * @brief The main loop using select() to handle I/O
 * @param sockfd The connected socket file descriptor
 */
void client_main_loop(int sockfd) {
    fd_set read_fds;
    int max_fd;

    printf("\n--- CLIENT STARTED ---\n");
    print_menu();
    printf("> ");
    fflush(stdout);

    while (1) {
        // 1. Setup for select()
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);       // Watch the socket (Server messages)
        FD_SET(STDIN_FILENO, &read_fds); // Watch the keyboard (User input)

        max_fd = sockfd; // STDIN is 0, so sockfd is usually larger

        // 2. Wait for activity (Blocking call)
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if ((activity < 0)) {
            perror("select error");
            break;
        }

        // 3. Check if Server sent data
        if (FD_ISSET(sockfd, &read_fds)) {
            handle_server_message(sockfd);
        }

        // 4. Check if User typed something
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            handle_user_input(sockfd);
            printf("> "); // Repaint prompt
            fflush(stdout);
        }
    }
}

// --- HANDLERS ---

/**
 * @brief Processes messages received from the server
 */
void handle_server_message(int sockfd) {
    int msg_type;
    char buffer[BUFFER_SIZE];

    // Use the recv_packet function from network.c
    int payload_len = recv_packet(sockfd, &msg_type, buffer);

    if (payload_len < 0) {
        printf("\nDisconnected from server.\n");
        exit(0);
    }

    // Clear the current line to prevent messing up the prompt "> "
    printf("\r\x1b[K"); 

    // Routing logic based on Message Type
    switch (msg_type) {
        case MSG_SUCCESS:
            printf("[SUCCESS] %s\n", buffer);
            break;

        case MSG_ERROR:
            printf("[ERROR] %s\n", buffer);
            break;

        case MSG_LOGIN: // Server might send back login info
            printf("[LOGIN] %s\n", buffer);
            break;

        case MSG_LIST_GROUPS:
            printf("--- GROUP LIST ---\n%s\n------------------\n", buffer);
            break;
        
        // Add other cases here (File transfer, etc.)
        
        default:
            printf("[INFO] Received MSG Type %d: %s\n", msg_type, buffer);
            break;
    }
    
    // Repaint prompt after message
    printf("> "); 
    fflush(stdout);
}

/**
 * @brief Processes keyboard input and sends packets
 */
void handle_user_input(int sockfd) {
    char input[BUFFER_SIZE];
    char command[50], arg1[100], arg2[100];

    // Read line from keyboard
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return; 
    }

    // Remove newline char
    input[strcspn(input, "\n")] = 0;

    if (strlen(input) == 0) return;

    // Parse command: splits "LOGIN user pass" -> cmd="LOGIN", arg1="user", arg2="pass"
    int args = sscanf(input, "%s %s %s", command, arg1, arg2);

    if (strcasecmp(command, "EXIT") == 0) {
        printf("Exiting...\n");
        close(sockfd);
        exit(0);
    }
    else if (strcasecmp(command, "LOGIN") == 0) {
        if (args < 3) {
            printf("Usage: LOGIN <username> <password>\n");
        } else {
            char payload[256];
            sprintf(payload, "%s %s", arg1, arg2);
            send_packet(sockfd, MSG_LOGIN, payload, strlen(payload));
        }
    }
    else if (strcasecmp(command, "REGISTER") == 0) {
        if (args < 3) {
            printf("Usage: REGISTER <username> <password>\n");
        } else {
            char payload[256];
            sprintf(payload, "%s %s", arg1, arg2);
            send_packet(sockfd, MSG_REGISTER, payload, strlen(payload));
        }
    }
    else if (strcasecmp(command, "CREATE_GROUP") == 0) {
        if (args < 2) {
            printf("Usage: CREATE_GROUP <group_name>\n");
        } else {
            send_packet(sockfd, MSG_CREATE_GROUP, arg1, strlen(arg1));
        }
    }
    else if (strcasecmp(command, "LIST_GROUPS") == 0) {
        send_packet(sockfd, MSG_LIST_GROUPS, "", 0);
    }
    // Add more commands here...
    else {
        printf("Unknown command. Type 'HELP' for menu.\n");
    }
}

void print_menu() {
    printf("--- COMMAND MENU ---\n");
    printf("1. LOGIN <user> <pass>\n");
    printf("2. REGISTER <user> <pass>\n");
    printf("3. CREATE_GROUP <name>\n");
    printf("4. LIST_GROUPS\n");
    printf("5. EXIT\n");
}