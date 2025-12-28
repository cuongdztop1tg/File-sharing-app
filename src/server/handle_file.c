#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "common.h"
#include "protocol.h"
#include "network.h"

#define FILE_STORAGE_PATH "./data/files/"

// 1. Hàm liệt kê file trong thư mục server
void handle_list_files(int sockfd) {
    DIR *d;
    struct dirent *dir;
    char file_list[BUFFER_SIZE] = "";

    d = opendir(FILE_STORAGE_PATH);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // Bỏ qua thư mục hiện tại (.) và thư mục cha (..)
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                strcat(file_list, dir->d_name);
                strcat(file_list, "\n");
            }
        }
        closedir(d);
        send_packet(sockfd, MSG_LIST_RESPONSE, file_list, strlen(file_list));
    } else {
        char *err = "Error: Cannot open file directory.";
        send_packet(sockfd, MSG_ERROR, err, strlen(err));
    }
}

// 2. Hàm xử lý nhận file từ Client (Upload)
void handle_upload_request(int sockfd, char *payload) {
    char filename[100];
    long filesize = 0;
    
    // Payload format: "filename filesize"
    sscanf(payload, "%s %ld", filename, &filesize);

    char filepath[200];
    sprintf(filepath, "%s%s", FILE_STORAGE_PATH, filename);

    FILE *f = fopen(filepath, "wb"); // Mở chế độ Binary write
    if (!f) {
        send_packet(sockfd, MSG_ERROR, "Server cannot create file", 25);
        return;
    }

    // Báo cho Client biết Server đã sẵn sàng nhận
    send_packet(sockfd, MSG_SUCCESS, "Ready to receive", 16);

    // --- BẮT ĐẦU VÒNG LẶP NHẬN FILE (Blocking) ---
    // Lưu ý: Server sẽ tạm thời chỉ phục vụ việc nhận file này 
    // cho client này cho đến khi xong.
    int msg_type;
    char buffer[BUFFER_SIZE];
    long total_received = 0;
    int payload_len;

    while (1) {
        payload_len = recv_packet(sockfd, &msg_type, buffer);
        
        if (payload_len < 0) break; // Error handling

        if (msg_type == MSG_FILE_DATA) {
            fwrite(buffer, 1, payload_len, f);
            total_received += payload_len;
        } 
        else if (msg_type == MSG_FILE_END) {
            printf("Upload completed: %s (%ld bytes)\n", filename, total_received);
            break; // Kết thúc
        }
        else {
            // Nhận tin nhắn rác hoặc lỗi
            break;
        }
    }
    
    fclose(f);
    
    // Gửi xác nhận cuối cùng
    char success_msg[100];
    sprintf(success_msg, "File uploaded successfully: %s", filename);
    send_packet(sockfd, MSG_SUCCESS, success_msg, strlen(success_msg));
}

void handle_download_request(int sockfd, char *filename) {
    char filepath[256];
    sprintf(filepath, "%s%s", FILE_STORAGE_PATH, filename);

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        char *err = "File not found.";
        send_packet(sockfd, MSG_ERROR, err, strlen(err));
        return;
    }

    // 1. Gửi thông báo chấp nhận Download + Kích thước file (để Client hiện thanh tiến trình nếu muốn)
    struct stat st;
    stat(filepath, &st);
    long filesize = st.st_size;
    
    char msg[100];
    sprintf(msg, "%ld", filesize);
    send_packet(sockfd, MSG_SUCCESS, msg, strlen(msg));

    // 2. Bắt đầu gửi dữ liệu
    printf("[INFO] Sending file '%s' to Client...\n", filename);
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        send_packet(sockfd, MSG_FILE_DATA, buffer, bytes_read);
    }
    
    // 3. Gửi tín hiệu kết thúc
    send_packet(sockfd, MSG_FILE_END, "", 0);
    
    fclose(f);
    printf("[INFO] File sent successfully.\n");
}

void handle_delete_file(int sockfd, char *filename) {
    char filepath[256];
    sprintf(filepath, "%s%s", FILE_STORAGE_PATH, filename);

    if (remove(filepath) == 0) {
        send_packet(sockfd, MSG_SUCCESS, "File deleted.", 13);
    } else {
        send_packet(sockfd, MSG_ERROR, "Cannot delete file.", 19);
    }
}

// Nếu muốn làm thêm Tạo thư mục (Module 3 có yêu cầu)
void handle_create_folder(int sockfd, char *foldername) {
    char path[256];
    sprintf(path, "%s%s", FILE_STORAGE_PATH, foldername);
    
    // 0777 là quyền truy cập (Read/Write/Execute)
#ifdef _WIN32
    if (_mkdir(path) == 0) // Windows dùng _mkdir
#else
    if (mkdir(path, 0777) == 0) // Linux/Mac dùng mkdir
#endif
    {
        send_packet(sockfd, MSG_SUCCESS, "Folder created.", 15);
    } else {
        send_packet(sockfd, MSG_ERROR, "Cannot create folder (Existed?).", 30);
    }
}