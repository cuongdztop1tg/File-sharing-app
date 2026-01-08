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
#include <sys/file.h>
#include "db.h"

#define FILE_STORAGE_PATH "./data/files/"


int remove_directory_recursive(const char *path);
Session *find_session(int sockfd);
void log_activity(const char *msg);
int remove_directory_recursive(const char *path);
int check_group_write_permission(int user_id, const char *path);
int check_group_owner_permission(int user_id, const char *path);

void get_log_prefix(int sockfd, char *buffer) {
    Session *s = find_session(sockfd);
    if (s) {
        sprintf(buffer, "[%s] User '%s' (ID %d)", s->client_ip, s->username, s->user_id);
    } else {
        sprintf(buffer, "[Unknown] Guest");
    }
}

int is_file_busy(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) return 0;

    int fd = fileno(f);
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        fclose(f);
        return 1;
    }

    flock(fd, LOCK_UN);
    fclose(f);
    return 0;
}

void handle_list_files(int sockfd, char *subpath) {

    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char log_msg[512];
    sprintf(log_msg, "%s requested LIST_FILES path='%s'", log_prefix, subpath ? subpath : "root");
    log_activity(log_msg);

    DIR *d;
    struct dirent *dir;
    char file_list[BUFFER_SIZE] = "";
    char full_path[512];
    

    if (subpath != NULL && strstr(subpath, "..")) {
        char *err = "Error: Invalid path (Access denied).";
        send_packet(sockfd, MSG_ERROR, err, strlen(err));
        // Log lỗi
        sprintf(log_msg, "%s - LIST failed: Access denied (..)", log_prefix);
        log_activity(log_msg);
        return;
    }

    if (subpath != NULL && strlen(subpath) > 0) {
        sprintf(full_path, "%s%s", FILE_STORAGE_PATH, subpath);
    } else {
        sprintf(full_path, "%s", FILE_STORAGE_PATH);
    }

    d = opendir(full_path);
    if (d) {
        sprintf(file_list, "--- Content of: /%s ---\n", (subpath ? subpath : "root"));

        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                strcat(file_list, dir->d_name);
                
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
        
        if (strlen(file_list) < 30) strcat(file_list, "(Empty folder)");

        send_packet(sockfd, MSG_LIST_RESPONSE, file_list, strlen(file_list));
    } else {
        char *err = "Error: Folder not found.";
        send_packet(sockfd, MSG_ERROR, err, strlen(err));
        sprintf(log_msg, "%s - LIST failed: Folder not found '%s'", log_prefix, subpath);
        log_activity(log_msg);
    }
}

void handle_upload_request(int sockfd, char *payload) {

    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char filename[100];
    long filesize = 0;
    
    sscanf(payload, "%s %ld", filename, &filesize);

    Session *s = find_session(sockfd);
    if (s && !check_group_write_permission(s->user_id, filename)) {
        send_packet(sockfd, MSG_ERROR, "Access Denied: You are not a member of this group", 48);
        char log_msg[512];
        sprintf(log_msg, "%s - UPLOAD denied to '%s' (Not a group member)", log_prefix, filename);
        log_activity(log_msg);
        return;
    }

    char log_msg[512];
    sprintf(log_msg, "%s requesting UPLOAD '%s' (%ld bytes)", log_prefix, filename, filesize);
    log_activity(log_msg);

    char filepath[200];
    sprintf(filepath, "%s%s", FILE_STORAGE_PATH, filename);

    FILE *f = fopen(filepath, "wb");
    if (!f) {
        send_packet(sockfd, MSG_ERROR, "Server cannot create file", 25);
        sprintf(log_msg, "%s - UPLOAD failed: Cannot create file on disk", log_prefix);
        log_activity(log_msg);
        return;
    }

    int fd = fileno(f);
    if (flock(fd, LOCK_EX) != 0) {
        perror("Lock failed");
        fclose(f);
    return;
    }
    

    send_packet(sockfd, MSG_SUCCESS, "Ready to receive", 16);

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
            break;
        }
        else {
            break;
        }
    }
    
    fclose(f);
    
    char success_msg[100];
    sprintf(success_msg, "File uploaded successfully: %s", filename);
    send_packet(sockfd, MSG_SUCCESS, success_msg, strlen(success_msg));

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

    struct stat st;
    if (stat(filepath, &st) != 0) {
        char *err = "File not found.";
        send_packet(sockfd, MSG_ERROR, err, strlen(err));
        sprintf(log_msg, "%s - DOWNLOAD failed: File not found", log_prefix);
        log_activity(log_msg);
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        char *err = "Cannot download a folder directly.";
        send_packet(sockfd, MSG_ERROR, err, strlen(err));
        sprintf(log_msg, "%s - DOWNLOAD failed: Target is a directory", log_prefix);
        log_activity(log_msg);
        return;
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        char *err = "Access denied or file locked.";
        send_packet(sockfd, MSG_ERROR, err, strlen(err));
        sprintf(log_msg, "%s - DOWNLOAD failed: Cannot open file", log_prefix);
        log_activity(log_msg);
        return;
    }
    int fd = fileno(f);
    if (flock(fd, LOCK_SH) != 0) {
    }

    long filesize = st.st_size;
    
    char msg[100];
    sprintf(msg, "%ld", filesize);
    send_packet(sockfd, MSG_SUCCESS, msg, strlen(msg));

    printf("[INFO] Sending file '%s' to Client...\n", filename);
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long total_sent = 0;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        send_packet(sockfd, MSG_FILE_DATA, buffer, bytes_read);
        total_sent += bytes_read;
    }
    
    send_packet(sockfd, MSG_FILE_END, "", 0);
    fclose(f);
    sprintf(log_msg, "%s - DOWNLOAD success: Sent '%s' (%ld bytes)", log_prefix, filename, total_sent);
    log_activity(log_msg);
    printf("[INFO] File sent successfully.\n");
}


void handle_delete_item(int sockfd, char *filename) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    Session *s = find_session(sockfd);
    if (s) {
        if (strstr(filename, "Group_")) {
            if (!check_group_owner_permission(s->user_id, filename)) {
                send_packet(sockfd, MSG_ERROR, "Access Denied: Only Group Owner can delete", 100);
                char log_msg[512];
                sprintf(log_msg, "%s - DELETE denied '%s' (Not owner)", s->username, filename);
                log_activity(log_msg);
                return;
            }
        }
    }
    
    char log_msg[512];
    sprintf(log_msg, "%s requested DELETE '%s'", log_prefix, filename);
    log_activity(log_msg);

    char filepath[256];
    sprintf(filepath, "%s%s", FILE_STORAGE_PATH, filename);

    

    struct stat st;
    if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
        if (is_file_busy(filepath)) {
            send_packet(sockfd, MSG_ERROR, "Cannot delete: File is being used by another user.", 50);
            return;
        }
    }
    
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


void handle_rename_item(int sockfd, char *payload) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char old_name[100], new_name[100];
    if (sscanf(payload, "%s %s", old_name, new_name) < 2) {
        send_packet(sockfd, MSG_ERROR, "Usage: RENAME <old> <new>", 25);
        return;
    }

    Session *s = find_session(sockfd);
    
    if (s) {
        if ((strstr(old_name, "Group_") || strstr(new_name, "Group_"))) {
             if (!check_group_owner_permission(s->user_id, old_name)) {
                send_packet(sockfd, MSG_ERROR, "Access Denied: Only Owner can rename group files", 100);
                return;
             }
        }
    }

    char log_msg[512];
    sprintf(log_msg, "%s requested RENAME '%s' -> '%s'", log_prefix, old_name, new_name);
    log_activity(log_msg);

    char old_path[512], new_path[512];
    sprintf(old_path, "%s%s", FILE_STORAGE_PATH, old_name);
    sprintf(new_path, "%s%s", FILE_STORAGE_PATH, new_name);

    // --- CHECK RACE CONDITION ---
    if (is_file_busy(old_path)) {
        send_packet(sockfd, MSG_ERROR, "Cannot rename: File is busy.", 27);
        return;
    }

    if (access(old_path, F_OK) != 0) {
        send_packet(sockfd, MSG_ERROR, "File/Folder not found", 21);
        sprintf(log_msg, "%s - RENAME failed", log_prefix);
        log_activity(log_msg);
        return;
    }

    if (access(new_path, F_OK) == 0) {
        send_packet(sockfd, MSG_ERROR, "New name already exists", 23);
        sprintf(log_msg, "%s - RENAME failed", log_prefix);
        log_activity(log_msg);
        return;
    }

    if (rename(old_path, new_path) == 0) {
        send_packet(sockfd, MSG_SUCCESS, "Rename successful", 17);
        sprintf(log_msg, "%s - RENAME success", log_prefix);
        log_activity(log_msg);
    } else {
        send_packet(sockfd, MSG_ERROR, "Rename failed", 13);
        sprintf(log_msg, "%s - RENAME failed", log_prefix);
        log_activity(log_msg);
    }
}

void handle_move_item(int sockfd, char *payload) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char src_name[100], dest_folder_input[100];
    
    if (sscanf(payload, "%s %s", src_name, dest_folder_input) < 2) {
        send_packet(sockfd, MSG_ERROR, "Usage: MOVE <source> <dest_folder>", 32);
        return;
    }
    Session *s = find_session(sockfd);
    
    if (s && !check_group_write_permission(s->user_id, src_name)) {
        send_packet(sockfd, MSG_ERROR, "Access Denied: You cannot move files from this group", 52);
        char log_msg[512];
        sprintf(log_msg, "%s - MOVE denied source '%s' (Not a group member)", log_prefix, src_name);
        log_activity(log_msg);
        return;
    }

    if (s && !check_group_write_permission(s->user_id, dest_folder_input)) {
        send_packet(sockfd, MSG_ERROR, "Access Denied: You cannot move files to this group", 50);
        char log_msg[512];
        sprintf(log_msg, "%s - MOVE denied dest '%s' (Not a group member)", log_prefix, dest_folder_input);
        log_activity(log_msg);
        return;
    }

    char src_path[PATH_MAX];
    snprintf(src_path, sizeof(src_path), "%s%s", FILE_STORAGE_PATH, src_name);

    struct stat st_src;
    if (stat(src_path, &st_src) != 0) {
        send_packet(sockfd, MSG_ERROR, "Source not found", 16);
        return;
    }

    if (S_ISREG(st_src.st_mode)) {
        if (is_file_busy(src_path)) {
            send_packet(sockfd, MSG_ERROR, "Cannot move: File is being used by another user.", 50);
            return;
        }
    }

    char raw_dest_path[PATH_MAX];
    snprintf(raw_dest_path, sizeof(raw_dest_path), "%s%s", FILE_STORAGE_PATH, dest_folder_input);

    char resolved_dest_path[PATH_MAX];
    char resolved_storage_root[PATH_MAX];

    if (realpath(FILE_STORAGE_PATH, resolved_storage_root) == NULL) {
        perror("Server Error: Cannot resolve storage root");
        return; 
    }

    if (realpath(raw_dest_path, resolved_dest_path) == NULL) {
        send_packet(sockfd, MSG_ERROR, "Destination folder not found or invalid", 50);
        return;
    }

    if (strncmp(resolved_dest_path, resolved_storage_root, strlen(resolved_storage_root)) != 0) {
        send_packet(sockfd, MSG_ERROR, "Access Denied: Cannot move out of storage root", 50);
        char log_msg[512];
        sprintf(log_msg, "%s - SECURITY ALERT: Attempted to move file outside root!", log_prefix);
        log_activity(log_msg);
        return;
    }

    char *filename_only = strrchr(src_name, '/');
    if (filename_only) filename_only++;
    else filename_only = src_name;

    char final_dest_path[PATH_MAX];
    snprintf(final_dest_path, sizeof(final_dest_path), "%s/%s", resolved_dest_path, filename_only);

    if (access(final_dest_path, F_OK) == 0) {
        send_packet(sockfd, MSG_ERROR, "Item already exists in destination", 50);
        return;
    }

    if (rename(src_path, final_dest_path) == 0) {
        send_packet(sockfd, MSG_SUCCESS, "Move successful", 15);
        
        char log_msg[512];
        sprintf(log_msg, "%s - MOVE success: '%s' -> '%s'", log_prefix, src_name, dest_folder_input);
        log_activity(log_msg);
    } else {
        if (errno == EINVAL) {
            send_packet(sockfd, MSG_ERROR, "Invalid move: Cannot move folder into itself", 50);
        } else if (errno == EXDEV) {
            send_packet(sockfd, MSG_ERROR, "Cannot move across different disk partitions", 50);
        } else {
            send_packet(sockfd, MSG_ERROR, "Move failed (System Error)", 50);
            perror("Move Error");
        }
    }
}

void handle_create_folder(int sockfd, char *foldername) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    Session *s = find_session(sockfd);
    if (s && !check_group_write_permission(s->user_id, foldername)) {
        send_packet(sockfd, MSG_ERROR, "Access Denied: You are not a member of this group", 48);
        char log_msg[512];
        sprintf(log_msg, "%s - MKDIR denied at '%s' (Not a group member)", log_prefix, foldername);
        log_activity(log_msg);
        return;
    }

    char log_msg[512];
    sprintf(log_msg, "%s requested CREATE FOLDER '%s'", log_prefix, foldername);
    log_activity(log_msg);

    char path[256];
    sprintf(path, "%s%s", FILE_STORAGE_PATH, foldername);
    
#ifdef _WIN32
    if (_mkdir(path) == 0)
#else
    if (mkdir(path, 0777) == 0)
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

            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2; 
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;
                snprintf(buf, len, "%s/%s", path, p->d_name);

                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = remove_directory_recursive(buf); 
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r)
        r = rmdir(path);
    return r;
}

int copy_single_file(const char *src_path, const char *dest_path) {
    FILE *f_src = fopen(src_path, "rb");
    if (!f_src) return -1;
    flock(fileno(f_src), LOCK_SH);

    FILE *f_dest = fopen(dest_path, "wb");
    if (!f_dest) {
        fclose(f_src);
        return -1;
    }
    flock(fileno(f_dest), LOCK_EX);

    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), f_src)) > 0) {
        if (fwrite(buffer, 1, n, f_dest) != n) {
            fclose(f_src);
            fclose(f_dest);
            return -1;
        }
    }

    fclose(f_src);
    fclose(f_dest);
    return 0;
}

int copy_recursive(const char *src, const char *dest) {
    struct stat st;
    if (stat(src, &st) != 0) return -1;

    if (S_ISREG(st.st_mode)) {
        return copy_single_file(src, dest);
    }

    if (S_ISDIR(st.st_mode)) {
        #ifdef _WIN32
            _mkdir(dest);
        #else
            mkdir(dest, 0755);
        #endif

        DIR *d = opendir(src);
        if (!d) return -1;

        struct dirent *entry;
        while ((entry = readdir(d)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char next_src[PATH_MAX];
            char next_dest[PATH_MAX];

            snprintf(next_src, sizeof(next_src), "%s/%s", src, entry->d_name);
            snprintf(next_dest, sizeof(next_dest), "%s/%s", dest, entry->d_name);

            copy_recursive(next_src, next_dest);
        }
        closedir(d);
        return 0;
    }
    return -1;
}


void handle_copy_file(int sockfd, char *payload) {
    char log_prefix[256];
    get_log_prefix(sockfd, log_prefix);

    char src_name[100], dest_input[100];
    if (sscanf(payload, "%s %s", src_name, dest_input) < 2) {
        send_packet(sockfd, MSG_ERROR, "Usage: COPY <source> <dest_path>", 30);
        return;
    }

    Session *s = find_session(sockfd);
    
    if (s && !check_group_write_permission(s->user_id, src_name)) {
         send_packet(sockfd, MSG_ERROR, "Access Denied: Source group restricted", 38);
         return;
    }

    if (s && !check_group_write_permission(s->user_id, dest_input)) {
         send_packet(sockfd, MSG_ERROR, "Access Denied: Destination group restricted", 43);
         return;
    }

    char src_path[PATH_MAX];
    snprintf(src_path, sizeof(src_path), "%s%s", FILE_STORAGE_PATH, src_name);

    struct stat st_src;
    if (stat(src_path, &st_src) != 0) {
        send_packet(sockfd, MSG_ERROR, "Source not found", 16);
        return;
    }

    char raw_dest_base[PATH_MAX];
    snprintf(raw_dest_base, sizeof(raw_dest_base), "%s%s", FILE_STORAGE_PATH, dest_input);

    char final_dest_path[PATH_MAX];
    struct stat st_dest;

    if (stat(raw_dest_base, &st_dest) == 0 && S_ISDIR(st_dest.st_mode)) {
        char *filename_only = strrchr(src_name, '/');
        if (filename_only) filename_only++; 
        else filename_only = src_name;
        
        snprintf(final_dest_path, sizeof(final_dest_path), "%s/%s", raw_dest_base, filename_only);
    } else {
        strcpy(final_dest_path, raw_dest_base);
    }

    char resolved_storage_root[PATH_MAX];
    realpath(FILE_STORAGE_PATH, resolved_storage_root);
    
    if (strstr(dest_input, "..")) {
         send_packet(sockfd, MSG_ERROR, "Security Violation: '..' not allowed", 32);
         return;
    }

    if (access(final_dest_path, F_OK) == 0) {
        send_packet(sockfd, MSG_ERROR, "Destination already exists (No Overwrite)", 40);
        return;
    }

    char resolved_src[PATH_MAX];
    if (realpath(src_path, resolved_src)) {
        char abs_dest[PATH_MAX];
        snprintf(abs_dest, sizeof(abs_dest), "%s/%s", resolved_storage_root, dest_input);
        
        if (strncmp(abs_dest, resolved_src, strlen(resolved_src)) == 0) {
             send_packet(sockfd, MSG_ERROR, "Cannot copy folder into its own subdirectory", 43);
             return;
        }
    }

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

int check_group_write_permission(int user_id, const char *path) {
    int group_id;
    
    if (sscanf(path, "Group_%d/", &group_id) == 1 || 
        (strncmp(path, "Group_", 6) == 0 && sscanf(path, "Group_%d", &group_id) == 1)) {
        
        GroupMemberInfo members[512];
        int count = db_read_group_members(members, 512);
        
        for (int i = 0; i < count; i++) {
            if (members[i].group_id == group_id && 
                members[i].user_id == user_id && 
                members[i].status == 1) {
                return 1;
            }
        }
        
        return 0; 
    }
    
    return 1; 
}


int check_group_owner_permission(int user_id, const char *path) {
    int group_id;
    
    if (sscanf(path, "Group_%d/", &group_id) == 1 || 
        (strncmp(path, "Group_", 6) == 0 && sscanf(path, "Group_%d", &group_id) == 1)) {
        
        GroupInfo groups[256];
        int count = db_read_groups(groups, 256);
        
        for (int i = 0; i < count; i++) {
            if (groups[i].group_id == group_id) {
                if (groups[i].owner_id == user_id) {
                    return 1;
                } else {
                    return 0;
                }
            }
        }
        return 0;
    }
    
    return 1; 
}