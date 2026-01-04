#include "protocol.h"
#include "network.h"
#include <stdio.h>

// External functions (Logic Handlers)

void handle_login(int sockfd, char *payload);
void handle_register(int sockfd, char *payload);
// void handle_create_group(int sockfd, char *payload); // Future implementation

// Khai báo prototype cho Module 3 (File Handling)
void handle_list_files(int sockfd, char *subpath);
void handle_upload_request(int sockfd, char *payload);
void handle_download_request(int sockfd, char *filename);
void handle_delete_item(int sockfd, char *filename);
void handle_create_folder(int sockfd, char *foldername);
void handle_copy_file(int sockfd, char *payload);
void handle_rename_item(int sockfd, char *payload);
void handle_move_item(int sockfd, char *payload); 
// --------------------------

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
        
        // --- MODULE 3: FILE HANDLING ---
        case MSG_LIST_FILES:
            handle_list_files(sockfd, payload); // Truyền payload (tên folder) vào hàm
            break;
        case MSG_UPLOAD_REQ:
            handle_upload_request(sockfd, payload);
            break;
        case MSG_DOWNLOAD_REQ:
            handle_download_request(sockfd, payload);
            break;
        case MSG_DELETE_ITEM:
            handle_delete_item(sockfd, payload); // Gọi hàm thực thi
            break;
        case MSG_CREATE_FOLDER:
            handle_create_folder(sockfd, payload);
            break;
        case MSG_RENAME_ITEM:
            handle_rename_item(sockfd, payload);
            break;
        case MSG_MOVE_ITEM:
            handle_move_item(sockfd, payload);
            break;
        case MSG_COPY_ITEM:
            handle_copy_file(sockfd, payload);
            break;

        // Add Group/File cases here...
        
        default:
            printf("Unknown message type: %d\n", msg_type);
            send_packet(sockfd, MSG_ERROR, "Unknown command", 15);
            break;
    }
}