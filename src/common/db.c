#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

// --- USER MANAGEMENT ---

/**
 * @brief Checks if username and password match a record in users.txt
 * @return UserID if success, -1 if failure.
 */
int db_check_login(const char *username, const char *password) {
    FILE *f = fopen(USER_DB_FILE, "r");
    if (!f) return -1;

    int id;
    char u[50], p[50];
    
    // Format in file: ID Username Password
    while (fscanf(f, "%d %s %s", &id, u, p) != EOF) {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
            fclose(f);
            return id; // Login success
        }
    }

    fclose(f);
    return -1; // Login failed
}

/**
 * @brief Registers a new user.
 * @return New UserID if success, -1 if username already exists.
 */
int db_register_user(const char *username, const char *password) {
    FILE *f = fopen(USER_DB_FILE, "a+"); // Read + Append
    if (!f) return -1;

    int id = 0, max_id = 0;
    char u[50], p[50];
    int exists = 0;

    // 1. Check for duplicates and find the highest ID
    fseek(f, 0, SEEK_SET); // Rewind to start
    while (fscanf(f, "%d %s %s", &id, u, p) != EOF) {
        if (strcmp(u, username) == 0) {
            exists = 1;
            break;
        }
        if (id > max_id) max_id = id;
    }

    if (exists) {
        fclose(f);
        return -1; // Error: User exists
    }

    // 2. Append new user
    int new_id = max_id + 1;
    // Move pointer to end just in case
    fseek(f, 0, SEEK_END); 
    fprintf(f, "%d %s %s\n", new_id, username, password);
    
    fclose(f);
    return new_id;
}

/**
 * @brief Đổi mật khẩu cho user ID tương ứng
 * @return 0 nếu thành công, -1 nếu thất bại
 */
int db_change_password(int user_id, const char *new_password) {
    FILE *f = fopen(USER_DB_FILE, "r");
    FILE *temp = fopen("users.tmp", "w");
    
    if (!f || !temp) return -1;

    int id;
    char u[50], p[50];
    int found = 0;

    // Đọc từng dòng từ file cũ
    while (fscanf(f, "%d %s %s", &id, u, p) != EOF) {
        if (id == user_id) {
            // Nếu khớp ID, ghi mật khẩu MỚI vào file tạm
            fprintf(temp, "%d %s %s\n", id, u, new_password);
            found = 1;
        } else {
            // Nếu không khớp, ghi lại dòng cũ
            fprintf(temp, "%d %s %s\n", id, u, p);
        }
    }

    fclose(f);
    fclose(temp);

    if (found) {
        remove(USER_DB_FILE);       // Xóa file cũ
        rename("users.tmp", USER_DB_FILE); // Đổi tên file tạm thành file chính
        return 0;
    } else {
        remove("users.tmp");
        return -1;
    }
}