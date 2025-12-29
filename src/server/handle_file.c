#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "common.h"
#include "protocol.h"
#include "network.h"
#include <unistd.h>

#define FILE_STORAGE_PATH "./data/files/"

// Khai báo trước hàm này để các hàm bên dưới có thể gọi nó
int remove_directory_recursive(const char *path);

// 1. Hàm liệt kê file trong thư mục server
void handle_list_files(int sockfd, char *subpath) {
    DIR *d;
    struct dirent *dir;
    char file_list[BUFFER_SIZE] = "";
    char full_path[512];

    // Bảo mật: Chặn người dùng truy cập ngược ra ngoài bằng ".."
    if (subpath != NULL && strstr(subpath, "..")) {
        char *err = "Error: Invalid path (Access denied).";
        send_packet(sockfd, MSG_ERROR, err, strlen(err));
        return;
    }

    // Xử lý đường dẫn: data/files/ + subpath
    // Nếu subpath rỗng hoặc null -> Liệt kê thư mục gốc
    if (subpath != NULL && strlen(subpath) > 0) {
        sprintf(full_path, "%s%s", FILE_STORAGE_PATH, subpath);
    } else {
        sprintf(full_path, "%s", FILE_STORAGE_PATH);
    }

    d = opendir(full_path);
    if (d) {
        // Thêm dòng tiêu đề để dễ nhìn
        sprintf(file_list, "--- Content of: /%s ---\n", (subpath ? subpath : "root"));

        while ((dir = readdir(d)) != NULL) {
            // Bỏ qua . và ..
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                strcat(file_list, dir->d_name);
                
                // Nếu là folder, thêm dấu / để dễ phân biệt
                struct stat st;
                char item_path[512];
                sprintf(item_path, "%s/%s", full_path, dir->d_name);
                if (stat(item_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                    strcat(file_list, "/");
                }
                
                strcat(file_list, "\n");
            }
        }
        closedir(d);
        
        // Nếu chuỗi rỗng (folder không có file gì)
        if (strlen(file_list) < 30) strcat(file_list, "(Empty folder)");

        send_packet(sockfd, MSG_LIST_RESPONSE, file_list, strlen(file_list));
    } else {
        char *err = "Error: Folder not found.";
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

// Cập nhật hàm handle_delete_file hiện tại của bạn
void handle_delete_item(int sockfd, char *filename) {
    char filepath[256];
    sprintf(filepath, "%s%s", FILE_STORAGE_PATH, filename);

    struct stat st;
    if (stat(filepath, &st) == 0 && S_ISDIR(st.st_mode)) {
        // Nếu là thư mục, gọi hàm xóa đệ quy
        if (remove_directory_recursive(filepath) == 0)
            send_packet(sockfd, MSG_SUCCESS, "Folder deleted", 14);
        else
            send_packet(sockfd, MSG_ERROR, "Cannot delete folder", 20);
    } else {
        // Nếu là file thường
        if (remove(filepath) == 0)
            send_packet(sockfd, MSG_SUCCESS, "File deleted", 12);
        else
            send_packet(sockfd, MSG_ERROR, "Cannot delete file", 18);
    }
}

void handle_rename_item(int sockfd, char *payload) {
    char old_name[100], new_name[100];
    
    // Giả sử client gửi "oldname newname" (tách nhau bằng khoảng trắng)
    // Nếu tên file có khoảng trắng, bạn cần xử lý tách chuỗi kỹ hơn.
    sscanf(payload, "%s %s", old_name, new_name);

    char old_path[256], new_path[256];
    sprintf(old_path, "%s%s", FILE_STORAGE_PATH, old_name);
    sprintf(new_path, "%s%s", FILE_STORAGE_PATH, new_name);

    if (rename(old_path, new_path) == 0) {
        send_packet(sockfd, MSG_SUCCESS, "Rename successful", 17);
    } else {
        send_packet(sockfd, MSG_ERROR, "Rename failed", 13);
    }
}


void handle_copy_file(int sockfd, char *payload) {
    char src_name[100], dest_name[100];
    sscanf(payload, "%s %s", src_name, dest_name);

    char src_path[256], dest_path[256];
    sprintf(src_path, "%s%s", FILE_STORAGE_PATH, src_name);
    sprintf(dest_path, "%s%s", FILE_STORAGE_PATH, dest_name);

    FILE *f_src = fopen(src_path, "rb");
    if (!f_src) {
        send_packet(sockfd, MSG_ERROR, "Source file not found", 21);
        return;
    }

    FILE *f_dest = fopen(dest_path, "wb");
    if (!f_dest) {
        fclose(f_src);
        send_packet(sockfd, MSG_ERROR, "Cannot create dest file", 23);
        return;
    }

    // Copy theo Chunk để tiết kiệm RAM
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f_src)) > 0) {
        fwrite(buffer, 1, bytes_read, f_dest);
    }

    fclose(f_src);
    fclose(f_dest);

    send_packet(sockfd, MSG_SUCCESS, "Copy successful", 15);
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


// Hàm phụ trợ để xóa đệ quy (không public ra ngoài, chỉ dùng nội bộ)
int remove_directory_recursive(const char *path) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;
        r = 0;
        while (!r && (p = readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            // Bỏ qua . và ..
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2; 
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;
                snprintf(buf, len, "%s/%s", path, p->d_name);

                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = remove_directory_recursive(buf); // Đệ quy nếu là folder
                    else
                        r2 = unlink(buf); // Xóa nếu là file
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r)
        r = rmdir(path); // Cuối cùng xóa thư mục rỗng
    return r;
}