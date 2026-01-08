#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "common.h"
#include "network.h"
#include "protocol.h"

// Hàm lấy kích thước file
long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) return st.st_size;
    return -1;
}

void upload_file(int sockfd, char *filename) {
    // 1. Kiểm tra file có tồn tại không
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("[ERROR] Cannot open file '%s'\n", filename);
        return;
    }
    long filesize = get_file_size(filename);

    // 2. Gửi yêu cầu Upload: "filename filesize"
    char req_payload[256];
    sprintf(req_payload, "%s %ld", filename, filesize);
    send_packet(sockfd, MSG_UPLOAD_REQ, req_payload, strlen(req_payload));

    // 3. Chờ Server xác nhận (MSG_SUCCESS)
    int msg_type;
    char response[BUFFER_SIZE];
    recv_packet(sockfd, &msg_type, response);

    if (msg_type != MSG_SUCCESS) {
        printf("[ERROR] Server denied upload. Reason: %s\n", response);
        fclose(f);
        return;
    }

    // 4. Bắt đầu gửi dữ liệu (Chunking)
    printf("[INFO] Uploading '%s' (%ld bytes)...\n", filename, filesize);
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        // Gửi từng chunk data
        send_packet(sockfd, MSG_FILE_DATA, buffer, bytes_read);
    }

    // 5. Gửi thông báo kết thúc
    send_packet(sockfd, MSG_FILE_END, "EOF", 3);
    
    fclose(f);
    printf("[INFO] File sent. Waiting for confirmation...\n");
}

void request_list_files(int sockfd) {
    send_packet(sockfd, MSG_LIST_FILES, "", 0);
}

void download_file(int sockfd, char *filename) {
    // 1. Gửi yêu cầu
    send_packet(sockfd, MSG_DOWNLOAD_REQ, filename, strlen(filename));

    // 2. Chờ phản hồi từ Server
    int msg_type;
    char buffer[BUFFER_SIZE + 1];
    int payload_len = recv_packet(sockfd, &msg_type, buffer);

    if (msg_type == MSG_ERROR) {
        printf("[ERROR] Download failed: %s\n", buffer);
        return;
    }

    char *save_name = strrchr(filename, '/');
    if (save_name) {
        save_name++; // Bỏ qua dấu '/'
    } else {
        save_name = filename; // Không có dấu '/' thì dùng nguyên tên
    }

    // Server đồng ý, payload chứa filesize (nếu cần dùng)
    printf("[INFO] Downloading '%s' from Server...\n", filename);

    // 3. Mở file để ghi (Dùng save_name thay vì filename gốc)
    FILE *f = fopen(save_name, "wb"); 
    if (!f) {
        printf("[ERROR] Cannot write file '%s' locally. Check permissions.\n", save_name);
        return;
    }


    long total_received = 0;
    
    // 4. Vòng lặp nhận dữ liệu
    while (1) {
        payload_len = recv_packet(sockfd, &msg_type, buffer);
        if (payload_len < 0) break; // Mất kết nối

        if (msg_type == MSG_FILE_DATA) {
            fwrite(buffer, 1, payload_len, f);
            total_received += payload_len;
            printf("\rReceived: %ld bytes", total_received); // Hiệu ứng loading đơn giản
            fflush(stdout);
        } else if (msg_type == MSG_FILE_END) {
            printf("\n[SUCCESS] File download completed!\n");
            break;
        } else {
            printf("\n[ERROR] Unexpected packet type: %d\n", msg_type);
            break;
        }
    }
    fclose(f);
}