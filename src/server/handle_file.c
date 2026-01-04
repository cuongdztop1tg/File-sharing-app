#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "common.h"
#include "protocol.h"
#include "network.h"
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#define FILE_STORAGE_PATH "./data/files/"

// Khai báo trước hàm này để các hàm bên dưới có thể gọi nó
int remove_directory_recursive(const char *path);
Session *find_session(int sockfd);
void log_activity(const char *msg);
int remove_directory_recursive(const char *path);

// --- HÀM HỖ TRỢ LOGGING ---
/**
 * @brief Lấy chuỗi thông tin client để ghi log (VD: [127.0.0.1] User 'admin' (ID 1))
 */
void get_log_prefix(int sockfd, char *buffer) {
    Session *s = find_session(sockfd);
    if (s) {
        // Nếu user đã login hoặc là guest
        sprintf(buffer, "[%s] User '%s' (ID %d)", s->client_ip, s->username, s->user_id);
    } else {
        // Trường hợp không tìm thấy session (hiếm gặp)
        sprintf(buffer, "[Unknown] Guest");
    }
}

// 1. Hàm liệt kê file trong thư mục server
void handle_list_files(int sockfd, char *subpath) {

    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char log_msg[512];
    sprintf(log_msg, "%s requested LIST_FILES path='%s'", log_prefix, subpath ? subpath : "root");
    log_activity(log_msg); // Ghi log yêu cầu

    DIR *d;
    struct dirent *dir;
    char file_list[BUFFER_SIZE] = "";
    char full_path[512];
    

    // Bảo mật: Chặn người dùng truy cập ngược ra ngoài bằng ".."
    if (subpath != NULL && strstr(subpath, "..")) {
        char *err = "Error: Invalid path (Access denied).";
        send_packet(sockfd, MSG_ERROR, err, strlen(err));
        // Log lỗi
        sprintf(log_msg, "%s - LIST failed: Access denied (..)", log_prefix);
        log_activity(log_msg);
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
        // Log lỗi
        sprintf(log_msg, "%s - LIST failed: Folder not found '%s'", log_prefix, subpath);
        log_activity(log_msg);
    }
}

// 2. Hàm xử lý nhận file từ Client (Upload)
void handle_upload_request(int sockfd, char *payload) {

    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char filename[100];
    long filesize = 0;
    
    // Payload format: "filename filesize"
    sscanf(payload, "%s %ld", filename, &filesize);

    char log_msg[512];
    sprintf(log_msg, "%s requesting UPLOAD '%s' (%ld bytes)", log_prefix, filename, filesize);
    log_activity(log_msg);

    char filepath[200];
    sprintf(filepath, "%s%s", FILE_STORAGE_PATH, filename);

    FILE *f = fopen(filepath, "wb"); // Mở chế độ Binary write
    if (!f) {
        send_packet(sockfd, MSG_ERROR, "Server cannot create file", 25);
        sprintf(log_msg, "%s - UPLOAD failed: Cannot create file on disk", log_prefix);
        log_activity(log_msg);
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
    // Log hoàn tất
    sprintf(log_msg, "%s - UPLOAD completed: '%s' (Received %ld bytes)", log_prefix, filename, total_received);
    log_activity(log_msg);
}

void handle_download_request(int sockfd, char *filename) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char log_msg[512];
    sprintf(log_msg, "%s requesting DOWNLOAD '%s'", log_prefix, filename);
    log_activity(log_msg);

    char filepath[256];
    sprintf(filepath, "%s%s", FILE_STORAGE_PATH, filename);

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        char *err = "File not found.";
        send_packet(sockfd, MSG_ERROR, err, strlen(err));

        sprintf(log_msg, "%s - DOWNLOAD failed: File not found", log_prefix);
        log_activity(log_msg);
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
    long total_sent = 0;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        send_packet(sockfd, MSG_FILE_DATA, buffer, bytes_read);
        total_sent += bytes_read;
    }
    
    // 3. Gửi tín hiệu kết thúc
    send_packet(sockfd, MSG_FILE_END, "", 0);
    fclose(f);
    sprintf(log_msg, "%s - DOWNLOAD success: Sent '%s' (%ld bytes)", log_prefix, filename, total_sent);
    log_activity(log_msg);
    printf("[INFO] File sent successfully.\n");
}

// Cập nhật hàm handle_delete_file hiện tại của bạn
void handle_delete_item(int sockfd, char *filename) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);
    
    char log_msg[512];
    sprintf(log_msg, "%s requested DELETE '%s'", log_prefix, filename);
    log_activity(log_msg);

    char filepath[256];
    sprintf(filepath, "%s%s", FILE_STORAGE_PATH, filename);

    struct stat st;
    if (stat(filepath, &st) == 0 && S_ISDIR(st.st_mode)) {
        // Nếu là thư mục, gọi hàm xóa đệ quy
        if (remove_directory_recursive(filepath) == 0){
            send_packet(sockfd, MSG_SUCCESS, "Folder deleted", 14);
            sprintf(log_msg, "%s - DELETE success (Folder): '%s'", log_prefix, filename);
            log_activity(log_msg);}
        else{
            send_packet(sockfd, MSG_ERROR, "Cannot delete folder", 20);
            sprintf(log_msg, "%s - DELETE failed (Folder): '%s'", log_prefix, filename);
            log_activity(log_msg);
            }
    } else {
        // Nếu là file thường
        if (remove(filepath) == 0){
            send_packet(sockfd, MSG_SUCCESS, "File deleted", 12);
            sprintf(log_msg, "%s - DELETE success (File): '%s'", log_prefix, filename);
            log_activity(log_msg);
        }
        else{
            send_packet(sockfd, MSG_ERROR, "Cannot delete file", 18);
            sprintf(log_msg, "%s - DELETE failed (File): '%s'", log_prefix, filename);
            log_activity(log_msg);
        }
    }
}

// void handle_rename_item(int sockfd, char *payload) {
//     char log_prefix[256];
//     get_log_prefix(sockfd, log_prefix);
//     char old_name[100], new_name[100];
    
//     // Giả sử client gửi "oldname newname" (tách nhau bằng khoảng trắng)
//     // Nếu tên file có khoảng trắng, bạn cần xử lý tách chuỗi kỹ hơn.
//     sscanf(payload, "%s %s", old_name, new_name);
//     char log_msg[512];
//     sprintf(log_msg, "%s requested RENAME/MOVE '%s' -> '%s'", log_prefix, old_name, new_name);
//     log_activity(log_msg);

//     char old_path[256], new_path[256];
//     sprintf(old_path, "%s%s", FILE_STORAGE_PATH, old_name);
//     sprintf(new_path, "%s%s", FILE_STORAGE_PATH, new_name);

//     if (rename(old_path, new_path) == 0) {
//         send_packet(sockfd, MSG_SUCCESS, "Rename successful", 17);
//         sprintf(log_msg, "%s - RENAME success", log_prefix);
//         log_activity(log_msg);
//     } else {
//         send_packet(sockfd, MSG_ERROR, "Rename failed", 13);
//         sprintf(log_msg, "%s - RENAME failed", log_prefix);
//         log_activity(log_msg);
//     }
// }


// --- XỬ LÝ 1: RENAME (Đổi tên tại chỗ) ---
void handle_rename_item(int sockfd, char *payload) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char old_name[100], new_name[100];
    if (sscanf(payload, "%s %s", old_name, new_name) < 2) {
        send_packet(sockfd, MSG_ERROR, "Usage: RENAME <old> <new>", 25);
        return;
    }

    char log_msg[512];
    sprintf(log_msg, "%s requested RENAME '%s' -> '%s'", log_prefix, old_name, new_name);
    log_activity(log_msg);

    char old_path[512], new_path[512];
    sprintf(old_path, "%s%s", FILE_STORAGE_PATH, old_name);
    sprintf(new_path, "%s%s", FILE_STORAGE_PATH, new_name);

    // 1. Check nguồn
    if (access(old_path, F_OK) != 0) {
        send_packet(sockfd, MSG_ERROR, "File/Folder not found", 21);
        sprintf(log_msg, "%s - RENAME failed", log_prefix);
        log_activity(log_msg);
        return;
    }

    // 2. Check đích (Chặn ghi đè)
    if (access(new_path, F_OK) == 0) {
        send_packet(sockfd, MSG_ERROR, "New name already exists", 23);
        sprintf(log_msg, "%s - RENAME failed", log_prefix);
        log_activity(log_msg);
        return;
    }

    // 3. Thực hiện
    if (rename(old_path, new_path) == 0) {
        send_packet(sockfd, MSG_SUCCESS, "Rename successful", 17);
        sprintf(log_msg, "%s - RENAME success", log_prefix);
        log_activity(log_msg);
    } else {
        send_packet(sockfd, MSG_ERROR, "Rename failed", 13);
        // perror("Rename error");
        sprintf(log_msg, "%s - RENAME failed", log_prefix);
        log_activity(log_msg);
    }
}

void handle_move_item(int sockfd, char *payload) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char src_name[100], dest_folder_input[100];
    
    // Payload: "nguồn  đích"
    // Ví dụ 1: "myfolder  backup_folder" (Move folder vào folder)
    // Ví dụ 2: "sub/file.txt  .."        (Move file ra ngoài 1 cấp so với thư mục sub)
    if (sscanf(payload, "%s %s", src_name, dest_folder_input) < 2) {
        send_packet(sockfd, MSG_ERROR, "Usage: MOVE <source> <dest_folder>", 32);
        return;
    }

    // 1. Dựng đường dẫn nguồn (Source Path)
    char src_path[PATH_MAX];
    snprintf(src_path, sizeof(src_path), "%s%s", FILE_STORAGE_PATH, src_name);

    // Kiểm tra nguồn tồn tại không
    struct stat st_src;
    if (stat(src_path, &st_src) != 0) {
        send_packet(sockfd, MSG_ERROR, "Source not found", 16);
        return;
    }

    // 2. Xử lý đường dẫn đích (Dest Path) & Hỗ trợ ".."
    // Bước A: Tạo đường dẫn thô
    char raw_dest_path[PATH_MAX];
    snprintf(raw_dest_path, sizeof(raw_dest_path), "%s%s", FILE_STORAGE_PATH, dest_folder_input);

    // Bước B: Dùng realpath để giải quyết các dấu ".." và "."
    char resolved_dest_path[PATH_MAX];
    char resolved_storage_root[PATH_MAX];

    // Lấy đường dẫn tuyệt đối của thư mục gốc để so sánh bảo mật
    if (realpath(FILE_STORAGE_PATH, resolved_storage_root) == NULL) {
        perror("Server Error: Cannot resolve storage root");
        return; 
    }

    // Lấy đường dẫn tuyệt đối của đích đến
    if (realpath(raw_dest_path, resolved_dest_path) == NULL) {
        // Nếu đích không tồn tại (realpath fail), báo lỗi ngay vì MOVE cần folder đích phải có trước
        send_packet(sockfd, MSG_ERROR, "Destination folder not found or invalid", 37);
        return;
    }

    // Bước C: BẢO MẬT - Chống Hack "Move ra ngoài Root"
    // So sánh xem đường dẫn đích có bắt đầu bằng đường dẫn gốc không
    if (strncmp(resolved_dest_path, resolved_storage_root, strlen(resolved_storage_root)) != 0) {
        send_packet(sockfd, MSG_ERROR, "Access Denied: Cannot move out of storage root", 46);
        char log_msg[512];
        sprintf(log_msg, "%s - SECURITY ALERT: Attempted to move file outside root!", log_prefix);
        log_activity(log_msg);
        return;
    }

    // 3. Tạo đường dẫn đích cuối cùng
    // Logic: Đích cuối = Thư mục đích (đã resolve) + / + Tên file/folder nguồn
    char *filename_only = strrchr(src_name, '/');
    if (filename_only) filename_only++; // Lấy phần tên sau dấu / cuối cùng
    else filename_only = src_name;

    char final_dest_path[PATH_MAX];
    snprintf(final_dest_path, sizeof(final_dest_path), "%s/%s", resolved_dest_path, filename_only);

    // 4. Kiểm tra trùng tên tại đích
    if (access(final_dest_path, F_OK) == 0) {
        send_packet(sockfd, MSG_ERROR, "Item already exists in destination", 34);
        return;
    }

    // 5. Thực hiện MOVE (Rename)
    // Hàm rename() của C xử lý tốt cả File và Folder
    if (rename(src_path, final_dest_path) == 0) {
        send_packet(sockfd, MSG_SUCCESS, "Move successful", 15);
        
        char log_msg[512];
        sprintf(log_msg, "%s - MOVE success: '%s' -> '%s'", log_prefix, src_name, dest_folder_input);
        log_activity(log_msg);
    } else {
        // Xử lý lỗi cụ thể
        if (errno == EINVAL) {
            // Lỗi này xảy ra khi cố move Folder Cha vào Folder Con của chính nó
            send_packet(sockfd, MSG_ERROR, "Invalid move: Cannot move folder into itself", 44);
        } else if (errno == EXDEV) {
            send_packet(sockfd, MSG_ERROR, "Cannot move across different disk partitions", 44);
        } else {
            send_packet(sockfd, MSG_ERROR, "Move failed (System Error)", 26);
            perror("Move Error");
        }
    }
}

// void handle_copy_file(int sockfd, char *payload) {
//     char log_prefix[256];
//     get_log_prefix(sockfd, log_prefix);

//     char src_name[100], dest_name[100];
//     sscanf(payload, "%s %s", src_name, dest_name);

//     char log_msg[512];
//     sprintf(log_msg, "%s requested COPY '%s' -> '%s'", log_prefix, src_name, dest_name);
//     log_activity(log_msg);

//     char src_path[256], dest_path[256];
//     sprintf(src_path, "%s%s", FILE_STORAGE_PATH, src_name);
//     sprintf(dest_path, "%s%s", FILE_STORAGE_PATH, dest_name);

//     FILE *f_src = fopen(src_path, "rb");
//     if (!f_src) {
//         send_packet(sockfd, MSG_ERROR, "Source file not found", 21);
//         sprintf(log_msg, "%s - COPY failed: Source not found", log_prefix);
//         log_activity(log_msg);
//         return;
//     }

//     FILE *f_dest = fopen(dest_path, "wb");
//     if (!f_dest) {
//         fclose(f_src);
//         send_packet(sockfd, MSG_ERROR, "Cannot create dest file", 23);
//         sprintf(log_msg, "%s - COPY failed: Cannot create dest file", log_prefix);
//         log_activity(log_msg);
//         return;
//     }

//     // Copy theo Chunk để tiết kiệm RAM
//     char buffer[BUFFER_SIZE];
//     size_t bytes_read;
//     while ((bytes_read = fread(buffer, 1, sizeof(buffer), f_src)) > 0) {
//         fwrite(buffer, 1, bytes_read, f_dest);
//     }

//     fclose(f_src);
//     fclose(f_dest);

//     send_packet(sockfd, MSG_SUCCESS, "Copy successful", 15);
//     sprintf(log_msg, "%s - COPY success", log_prefix);
//     log_activity(log_msg);
// }

// Nếu muốn làm thêm Tạo thư mục (Module 3 có yêu cầu)
void handle_create_folder(int sockfd, char *foldername) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char log_msg[512];
    sprintf(log_msg, "%s requested CREATE FOLDER '%s'", log_prefix, foldername);
    log_activity(log_msg);

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
        sprintf(log_msg, "%s - MKDIR success", log_prefix);
        log_activity(log_msg);
    } else {
        send_packet(sockfd, MSG_ERROR, "Cannot create folder (Existed?).", 30);
        sprintf(log_msg, "%s - MKDIR failed", log_prefix);
        log_activity(log_msg);
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

int copy_single_file(const char *src_path, const char *dest_path) {
    FILE *f_src = fopen(src_path, "rb");
    if (!f_src) return -1;

    FILE *f_dest = fopen(dest_path, "wb");
    if (!f_dest) {
        fclose(f_src);
        return -1;
    }

    char buffer[4096]; // Chunk 4KB
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), f_src)) > 0) {
        if (fwrite(buffer, 1, n, f_dest) != n) {
            fclose(f_src);
            fclose(f_dest);
            return -1; // Lỗi ghi đĩa đầy hoặc lỗi io
        }
    }

    fclose(f_src);
    fclose(f_dest);
    return 0; // Success
}

// Hàm đệ quy: Copy folder và toàn bộ nội dung
int copy_recursive(const char *src, const char *dest) {
    struct stat st;
    if (stat(src, &st) != 0) return -1;

    // 1. Nếu là File -> Copy file thường
    if (S_ISREG(st.st_mode)) {
        return copy_single_file(src, dest);
    }

    // 2. Nếu là Folder -> Tạo folder đích và đệ quy
    if (S_ISDIR(st.st_mode)) {
        // Tạo folder đích (Linux: 0755, Win: _mkdir)
        #ifdef _WIN32
            _mkdir(dest);
        #else
            mkdir(dest, 0755);
        #endif

        DIR *d = opendir(src);
        if (!d) return -1;

        struct dirent *entry;
        while ((entry = readdir(d)) != NULL) {
            // Bỏ qua . và ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char next_src[PATH_MAX];
            char next_dest[PATH_MAX];

            snprintf(next_src, sizeof(next_src), "%s/%s", src, entry->d_name);
            snprintf(next_dest, sizeof(next_dest), "%s/%s", dest, entry->d_name);

            // Gọi đệ quy
            copy_recursive(next_src, next_dest);
        }
        closedir(d);
        return 0;
    }
    return -1; // Không phải file/folder (VD: symlink, device...)
}


// --- XỬ LÝ 3: COPY (Đệ quy & An toàn) ---
void handle_copy_file(int sockfd, char *payload) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char src_name[100], dest_input[100];
    // Payload: "nguồn  đích"
    if (sscanf(payload, "%s %s", src_name, dest_input) < 2) {
        send_packet(sockfd, MSG_ERROR, "Usage: COPY <source> <dest_path>", 30);
        return;
    }

    // 1. Resolve Source Path
    char src_path[PATH_MAX];
    snprintf(src_path, sizeof(src_path), "%s%s", FILE_STORAGE_PATH, src_name);

    struct stat st_src;
    if (stat(src_path, &st_src) != 0) {
        send_packet(sockfd, MSG_ERROR, "Source not found", 16);
        return;
    }

    // 2. Resolve Destination Path
    char raw_dest_base[PATH_MAX];
    snprintf(raw_dest_base, sizeof(raw_dest_base), "%s%s", FILE_STORAGE_PATH, dest_input);

    char final_dest_path[PATH_MAX];
    struct stat st_dest;

    // Logic xác định đích đến cuối cùng
    if (stat(raw_dest_base, &st_dest) == 0 && S_ISDIR(st_dest.st_mode)) {
        // Nếu đích là Folder có sẵn -> Copy vào bên trong nó
        char *filename_only = strrchr(src_name, '/');
        if (filename_only) filename_only++; 
        else filename_only = src_name;
        
        snprintf(final_dest_path, sizeof(final_dest_path), "%s/%s", raw_dest_base, filename_only);
    } else {
        // Đích chưa tồn tại (Rename copy) hoặc là file
        strcpy(final_dest_path, raw_dest_base);
    }

    // 3. Bảo mật: Chống hack ".."
    char resolved_storage_root[PATH_MAX];
    realpath(FILE_STORAGE_PATH, resolved_storage_root);
    
    // [FIX]: Đã xóa dòng 'char resolved_final_dest[PATH_MAX];' gây warning ở đây
    
    // Kiểm tra nhanh xem user có dùng ".." để leo ra ngoài không
    if (strstr(dest_input, "..")) {
         send_packet(sockfd, MSG_ERROR, "Security Violation: '..' not allowed", 32);
         return;
    }

    // 4. NGOẠI LỆ 1: Check Trùng Lặp
    if (access(final_dest_path, F_OK) == 0) {
        send_packet(sockfd, MSG_ERROR, "Destination already exists (No Overwrite)", 40);
        return;
    }

    // 5. NGOẠI LỆ 2: Copy Folder vào con của chính nó (Infinite Loop)
    char resolved_src[PATH_MAX];
    if (realpath(src_path, resolved_src)) {
        char abs_dest[PATH_MAX];
        // Giả lập đường dẫn tuyệt đối của đích
        snprintf(abs_dest, sizeof(abs_dest), "%s/%s", resolved_storage_root, dest_input);
        
        // Nếu đường dẫn đích bắt đầu bằng đường dẫn nguồn -> Lỗi
        if (strncmp(abs_dest, resolved_src, strlen(resolved_src)) == 0) {
             send_packet(sockfd, MSG_ERROR, "Cannot copy folder into its own subdirectory", 43);
             return;
        }
    }

    // 6. Thực hiện Copy
    char log_msg[512];
    sprintf(log_msg, "%s requesting COPY '%s' -> '%s'", log_prefix, src_name, dest_input);
    log_activity(log_msg);

    if (copy_recursive(src_path, final_dest_path) == 0) {
        send_packet(sockfd, MSG_SUCCESS, "Copy successful", 15);
        sprintf(log_msg, "%s - COPY success", log_prefix);
        log_activity(log_msg);
    } else {
        send_packet(sockfd, MSG_ERROR, "Copy failed (IO Error)", 22);
        sprintf(log_msg, "%s - COPY failed", log_prefix);
        log_activity(log_msg);
    }
}