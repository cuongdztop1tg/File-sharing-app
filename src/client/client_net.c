#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "common.h"
#include "network.h"
#include "protocol.h"
#include "client.h"

// --- FUNCTION PROTOTYPES ---
// Handler for messages received from the server
void handle_server_message(int sockfd);

// Handler for processing and forwarding user input
void handle_user_input(int sockfd);

// Prints the initial menu displayed when client starts
void print_menu();

// Uploads a file to the server
void upload_file(int sockfd, char *filename);
// Downloads a file from the server
void download_file(int sockfd, char *filename);

// External function from client_ui.c to print full logged-in menu
void print_logged_in_menu();

// --- MAIN LOOP ---

/**
 * @brief The main loop using select() to handle I/O multiplexing between stdin and the server socket.
 * @param sockfd The connected socket file descriptor
 */
void client_main_loop(int sockfd)
{
    fd_set read_fds;
    int max_fd;

    printf("\n--- CLIENT STARTED ---\n");
    print_menu();
    printf("> "); // Print initial prompt
    fflush(stdout);

    while (1)
    {
        // 1. Setup read_fds for select()
        FD_ZERO(&read_fds);              // Clear the set
        FD_SET(sockfd, &read_fds);       // Monitor socket for server messages
        FD_SET(STDIN_FILENO, &read_fds); // Monitor standard input (keyboard)

        max_fd = sockfd; // Usually, STDIN_FILENO (0) is less than sockfd

        // 2. Wait for activity from either server socket or user input (blocking)
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if ((activity < 0))
        {
            perror("select error");
            break; // Exit loop on failure
        }

        // 3. Check if the server sent data
        if (FD_ISSET(sockfd, &read_fds))
        {
            handle_server_message(sockfd);
        }

        // 4. Check if the user typed something
        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            handle_user_input(sockfd);
            printf("> "); // Repaint prompt after processing user command
            fflush(stdout);
        }
    }
}

// --- HANDLERS ---

/**
 * @brief Processes messages received from the server and prints appropriate output.
 *        Clears the prompt before printing the server message.
 */
void handle_server_message(int sockfd)
{
    int msg_type;
    char buffer[BUFFER_SIZE];

    // Receive a packet from the server (see network.c for implementation)
    int payload_len = recv_packet(sockfd, &msg_type, buffer);

    if (payload_len < 0)
    {
        // Received error or disconnection
        printf("\nDisconnected from server.\n");
        exit(0);
    }

    // Clear the current line to prevent prompt overlap
    printf("\r\x1b[K");

    // Print message to user based on the type received
    switch (msg_type)
    {
    case MSG_SUCCESS:
        printf("[SUCCESS] %s\n", buffer);
        break;

    case MSG_ERROR:
        printf("[ERROR] %s\n", buffer);
        break;

    case MSG_LOGIN: // Server responds to login, possible data in buffer
        printf("[LOGIN] %s\n", buffer);
        break;

    case MSG_LIST_GROUPS:
        printf("\n--- GROUP LIST ---\n%s\n------------------\n", buffer);
        break;

    case MSG_LIST_RESPONSE:
        printf("\n--- FILE/FOLDER LIST ---\n%s\n--------------------\n", buffer);
        break;

    default:
        printf("[INFO] Received MSG Type %d: %s\n", msg_type, buffer);
        break;
    }

    // Print prompt again after server message
    printf("> ");
    fflush(stdout);
}

/**
 * @brief Processes keyboard input and sends corresponding packets/requests to the server.
 *
 *        Recognizes many command patterns. Prints usage hints for invalid commands.
 *        On "UPLOAD"/"DOWNLOAD", calls file transfer helpers.
 */
void handle_user_input(int sockfd)
{
    char input[BUFFER_SIZE];
    char command[50], arg1[100], arg2[100];

    // Read line of input from user (include input overflow protection)
    if (fgets(input, sizeof(input), stdin) == NULL)
    {
        return; // EOF or error, do nothing
    }

    // Remove trailing newline if present
    input[strcspn(input, "\n")] = 0;

    if (strlen(input) == 0)
        return; // Ignore empty lines

    // Parse up to 3 tokens: command, arg1, arg2
    int args = sscanf(input, "%s %s %s", command, arg1, arg2);

    // --- SYSTEM COMMANDS ---

    // Handle "EXIT" immediately (closes connection and exits)
    if (strcasecmp(command, "EXIT") == 0)
    {
        printf("Exiting...\n");
        close(sockfd);
        exit(0);
    }
    // "HELP" prints full logged-in menu
    else if (strcasecmp(command, "HELP") == 0)
    {
        print_logged_in_menu();
    }

    // --- AUTHENTICATION COMMANDS ---
    else if (strcasecmp(command, "LOGIN") == 0)
    {
        if (args < 3)
        {
            printf("Usage: LOGIN <username> <password>\n");
        }
        else
        {
            // Combine username and password, then send login packet
            char payload[256];
            sprintf(payload, "%s %s", arg1, arg2);
            send_packet(sockfd, MSG_LOGIN, payload, strlen(payload));
        }
    }
    else if (strcasecmp(command, "REGISTER") == 0)
    {
        if (args < 3)
        {
            printf("Usage: REGISTER <username> <password>\n");
        }
        else
        {
            // Combine username and password, then send registration packet
            char payload[256];
            sprintf(payload, "%s %s", arg1, arg2);
            send_packet(sockfd, MSG_REGISTER, payload, strlen(payload));
        }
    }
    else if (strcasecmp(command, "LOGOUT") == 0)
    {
        // Log out current user
        send_packet(sockfd, MSG_LOGOUT, "", 0);
        printf("Logged out.\n");
    }

    // --- GROUP MANAGEMENT COMMANDS ---

    // Create a new group
    else if (strcasecmp(command, "CREATE_GROUP") == 0)
    {
        if (args < 2)
        {
            printf("Usage: CREATE_GROUP <group_name>\n");
        }
        else
        {
            send_packet(sockfd, MSG_CREATE_GROUP, arg1, strlen(arg1));
        }
    }
    // List all groups
    else if (strcasecmp(command, "LIST_GROUPS") == 0)
    {
        send_packet(sockfd, MSG_LIST_GROUPS, "", 0);
    }
    // Join an existing group
    else if (strcasecmp(command, "JOIN_GROUP") == 0)
    {
        if (args < 2)
        {
            printf("Usage: JOIN_GROUP <group_id>\n");
        }
        else
        {
            send_packet(sockfd, MSG_JOIN_GROUP, arg1, strlen(arg1));
        }
    }
    // Leave a group
    else if (strcasecmp(command, "LEAVE_GROUP") == 0)
    {
        if (args < 2)
        {
            printf("Usage: LEAVE_GROUP <group_id>\n");
        }
        else
        {
            send_packet(sockfd, MSG_LEAVE_GROUP, arg1, strlen(arg1));
        }
    }
    // List members of a group
    else if (strcasecmp(command, "LIST_MEMBERS") == 0)
    {
        if (args < 2)
        {
            printf("Usage: LIST_MEMBERS <group_id>\n");
        }
        else
        {
            send_packet(sockfd, MSG_LIST_MEMBERS, arg1, strlen(arg1));
        }
    }
    // Kick a member from a group
    else if (strcasecmp(command, "KICK_MEMBER") == 0)
    {
        if (args < 3)
        {
            printf("Usage: KICK_MEMBER <group_id> <user_id>\n");
        }
        else
        {
            char payload[256];
            sprintf(payload, "%s %s", arg1, arg2);
            send_packet(sockfd, MSG_KICK_MEMBER, payload, strlen(payload));
        }
    }
    // Invite a user to a group
    else if (strcasecmp(command, "INVITE_MEMBER") == 0)
    {
        if (args < 3)
        {
            printf("Usage: INVITE_MEMBER <group_id> <user_id>\n");
        }
        else
        {
            char payload[256];
            sprintf(payload, "%s %s", arg1, arg2);
            send_packet(sockfd, MSG_INVITE_MEMBER, payload, strlen(payload));
        }
    }
    // Approve a user's group join request
    else if (strcasecmp(command, "APPROVE_MEMBER") == 0)
    {
        if (args < 3)
        {
            printf("Usage: APPROVE_MEMBER <group_id> <user_id>\n");
        }
        else
        {
            char payload[256];
            sprintf(payload, "%s %s", arg1, arg2);
            send_packet(sockfd, MSG_APPROVE_MEMBER, payload, strlen(payload));
        }
    }

    // --- FILE MANAGEMENT COMMANDS ---
    // List files/folders (with optional path arg)
    else if (strcasecmp(command, "LIST") == 0)
    {
        // If user types "LIST subfolder" (args = 2) -> Send "subfolder"
        if (args >= 2)
        {
            send_packet(sockfd, MSG_LIST_FILES, arg1, strlen(arg1));
        }
        // If user only types "LIST" -> Send empty string ""
        else
        {
            send_packet(sockfd, MSG_LIST_FILES, "", 0);
        }
    }
    // Upload file to server
    else if (strcasecmp(command, "UPLOAD") == 0)
    {
        if (args < 2)
        {
            printf("Usage: UPLOAD <filename>\n");
        }
        else
        {
            upload_file(sockfd, arg1);
        }
    }
    // Download file from server
    else if (strcasecmp(command, "DOWNLOAD") == 0)
    {
        if (args < 2)
        {
            printf("Usage: DOWNLOAD <filename>\n");
        }
        else
        {
            download_file(sockfd, arg1);
        }
    }
    // Delete a file or folder
    else if (strcasecmp(command, "DELETE") == 0)
    {
        if (args < 2)
        {
            printf("Usage: DELETE <filename>\n");
        }
        else
        {
            send_packet(sockfd, MSG_DELETE_ITEM, arg1, strlen(arg1));
        }
    }
    // Create a folder
    else if (strcasecmp(command, "MKDIR") == 0)
    {
        if (args < 2)
        {
            printf("Usage: MKDIR <foldername>\n");
        }
        else
        {
            send_packet(sockfd, MSG_CREATE_FOLDER, arg1, strlen(arg1));
        }
    }
    // Rename a file or folder
    else if (strcasecmp(command, "RENAME") == 0)
    {
        if (args < 3)
        {
            printf("Usage: RENAME <old_name> <new_name>\n");
        }
        else
        {
            char payload[256];
            sprintf(payload, "%s %s", arg1, arg2);
            send_packet(sockfd, MSG_RENAME_ITEM, payload, strlen(payload));
        }
    }
    // Copy a file or folder
    else if (strcasecmp(command, "COPY") == 0)
    {
        if (args < 3)
        {
            printf("Usage: COPY <source_file> <dest_file>\n");
        }
        else
        {
            char payload[256];
            sprintf(payload, "%s %s", arg1, arg2);
            send_packet(sockfd, MSG_COPY_ITEM, payload, strlen(payload));
        }
    }
    // Move a file or folder
    else if (strcasecmp(command, "MOVE") == 0)
    {
        if (args < 3)
        {
            printf("Usage: MOVE <source> <dest>\n");
        }
        else
        {
            char payload[256];
            sprintf(payload, "%s %s", arg1, arg2);
            send_packet(sockfd, MSG_MOVE_ITEM, payload, strlen(payload));
        }
    }

    // --- UNKNOWN COMMAND ---
    else
    {
        // All recognized commands are handled above. If reached, unknown.
        printf("Unknown command '%s'. Type 'HELP' for available commands.\n", command);
    }
}

/**
 * @brief Prints a brief command menu to help the user get started.
 */
void print_menu()
{
    printf("--- COMMAND MENU ---\n");
    printf("1. LOGIN <user> <pass>\n");
    printf("2. REGISTER <user> <pass>\n");
    printf("3. Type 'HELP' after login for more commands\n");
    printf("4. EXIT\n");
}

/**
 * @brief Uploads a file to the server.
 *        Notifies the server of new upload, then sends contents in chunks.
 *        Shows simple progress information.
 */
void upload_file(int sockfd, char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("[ERROR] Cannot open file '%s'\n", filename);
        return;
    }

    // Get total file size by seeking to end and using ftell
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Send upload request: "filename size"
    char request[256];
    sprintf(request, "%s %ld", filename, filesize);
    send_packet(sockfd, MSG_UPLOAD_REQ, request, strlen(request));

    printf("[INFO] Uploading '%s' (%ld bytes)...\n", filename, filesize);

    // Wait for server to acknowledge the upload request
    int msg_type;
    char buffer[BUFFER_SIZE];
    int len = recv_packet(sockfd, &msg_type, buffer);

    // If server didn't accept, abort
    if (len < 0 || msg_type != MSG_SUCCESS)
    {
        printf("[ERROR] Server rejected upload.\n");
        fclose(f);
        return;
    }

    // Send file contents data in BUFFER_SIZE chunks
    size_t bytes_read;
    long total_sent = 0;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0)
    {
        send_packet(sockfd, MSG_FILE_DATA, buffer, bytes_read);
        total_sent += bytes_read;

        // Show in-place upload progress
        printf("\rProgress: %ld / %ld bytes", total_sent, filesize);
        fflush(stdout);
    }

    // Send "file end" signal to tell the server upload is finished
    send_packet(sockfd, MSG_FILE_END, "", 0);
    fclose(f);

    printf("\n[INFO] Upload completed.\n");
}

/**
 * @brief Downloads a file from the server.
 *        Requests file, receives and writes contents, provides progress.
 */
void download_file(int sockfd, char *filename)
{
    // Send download request for given filename
    send_packet(sockfd, MSG_DOWNLOAD_REQ, filename, strlen(filename));

    // Wait for server's response (should contain file size or error)
    int msg_type;
    char buffer[BUFFER_SIZE];
    int len = recv_packet(sockfd, &msg_type, buffer);

    // On error, print server error message
    if (len < 0 || msg_type == MSG_ERROR)
    {
        printf("[ERROR] %s\n", buffer);
        return;
    }

    // Parse expected file size
    long filesize = atol(buffer);
    printf("[INFO] Downloading '%s' (%ld bytes)...\n", filename, filesize);

    // Open file (to write received data)
    FILE *f = fopen(filename, "wb");
    if (!f)
    {
        printf("[ERROR] Cannot create file '%s'\n", filename);
        return;
    }

    // Main receiving loop: receive data blocks from server and write to disk
    long total_received = 0;
    while (1)
    {
        len = recv_packet(sockfd, &msg_type, buffer);

        if (len < 0)
        {
            printf("\n[ERROR] Connection lost.\n");
            break;
        }

        if (msg_type == MSG_FILE_DATA)
        {
            // Write received block to file
            fwrite(buffer, 1, len, f);
            total_received += len;

            // Show in-place download progress indicator
            printf("\rProgress: %ld / %ld bytes", total_received, filesize);
            fflush(stdout);
        }
        else if (msg_type == MSG_FILE_END)
        {
            // Download finished
            printf("\n[INFO] Download completed.\n");
            break;
        }
        else
        {
            // Unexpected message, abort
            printf("\n[ERROR] Unexpected message type.\n");
            break;
        }
    }

    // Close output file
    fclose(f);
}