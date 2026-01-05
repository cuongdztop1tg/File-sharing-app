#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "common.h"

// --- USER MANAGEMENT ---

/**
 * @brief Checks if username and password match a record in users.txt
 * @return UserID if success, -1 if failure.
 */
int db_check_login(const char *username, const char *password)
{
    FILE *f = fopen(USER_DB_FILE, "r");
    if (!f)
        return -1;

    int id;
    char u[50], p[50];

    // Format in file: ID Username Password
    while (fscanf(f, "%d %s %s", &id, u, p) != EOF)
    {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0)
        {
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
int db_register_user(const char *username, const char *password)
{
    FILE *f = fopen(USER_DB_FILE, "a+"); // Read + Append
    if (!f)
        return -1;

    int id = 0, max_id = 0;
    char u[50], p[50];
    int exists = 0;

    // 1. Check for duplicates and find the highest ID
    fseek(f, 0, SEEK_SET); // Rewind to start
    while (fscanf(f, "%d %s %s", &id, u, p) != EOF)
    {
        if (strcmp(u, username) == 0)
        {
            exists = 1;
            break;
        }
        if (id > max_id)
            max_id = id;
    }

    if (exists)
    {
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

/**
 * @brief Xóa user khỏi database
 */
int db_delete_user(int user_id) {
    FILE *f = fopen(USER_DB_FILE, "r");
    FILE *temp = fopen("users.tmp", "w");
    if (!f || !temp) return -1;

    int id;
    char u[50], p[50];
    int found = 0;

    while (fscanf(f, "%d %s %s", &id, u, p) != EOF) {
        if (id == user_id) {
            found = 1; // Bỏ qua dòng này (không ghi vào temp)
        } else {
            fprintf(temp, "%d %s %s\n", id, u, p);
        }
    }

    fclose(f);
    fclose(temp);

    if (found) {
        remove(USER_DB_FILE);
        rename("users.tmp", USER_DB_FILE);
        return 0;
    } else {
        remove("users.tmp");
        return -1;
    }
}

//--------- GROUP MANAGEMENT ---------
// Helper struct for group info
// Only allow 512 users

typedef struct
{
    int group_id;
    char name[64];
    int owner_id;
} GroupInfo;

typedef struct
{
    int group_id;
    int user_id;
    int status;
} GroupMemberInfo; // 0: Pending, 1: Accepted

// NOTE ON PATHS:
// The server binary runs from the project root (./bin/server), so the working directory
// is the project root. All DB file paths use "./data/..." to match USER_DB_FILE.

// Reads all groups from "./data/groups.txt"
// Returns: count of groups loaded, or -1 on error
// Param: groups: pointer to array of GroupInfo, max_count: maximum number of groups to read
int db_read_groups(GroupInfo *groups, int max_count)
{
    FILE *f = fopen("./data/groups.txt", "r");
    if (!f)
        return -1;
    int count = 0;
    while (count < max_count && fscanf(f, "%d %63s %d", &groups[count].group_id, groups[count].name, &groups[count].owner_id) == 3)
    {
        count++;
    }
    fclose(f);
    return count;
}

// Appends a group to "./data/groups.txt"
// Returns 0 on success, -1 on error
// Param: group: pointer to GroupInfo (group_id, name, owner_id)
int db_write_group(const GroupInfo *group)
{
    // Ensure data directory exists
    struct stat st = {0};
    if (stat("./data", &st) == -1)
    {
        // Directory doesn't exist, try to create it
        if (mkdir("./data", 0755) != 0 && errno != EEXIST)
        {
            return -1; // Failed to create directory
        }
    }

    FILE *f = fopen("./data/groups.txt", "a");
    if (!f)
        return -1;
    fprintf(f, "%d %s %d\n", group->group_id, group->name, group->owner_id);
    fclose(f);
    return 0;
}

// Reads all group members from "./data/group_members.txt"
// Returns: count of members loaded, or -1 on error
// Param: members: pointer to array of GroupMemberInfo, max_count: maximum number of members to read
int db_read_group_members(GroupMemberInfo *members, int max_count)
{
    FILE *f = fopen("./data/group_members.txt", "r");
    if (!f)
        return -1;
    int count = 0;
    while (count < max_count && fscanf(f, "%d %d %d", &members[count].group_id, &members[count].user_id, &members[count].status) == 3)
    {
        count++;
    }
    fclose(f);
    return count;
}

// Appends a member to "./data/group_members.txt"
// Returns 0 on success, -1 on error
// Param: member: pointer to GroupMemberInfo
int db_write_group_member(const GroupMemberInfo *member)
{
    // Ensure data directory exists
    struct stat st = {0};
    if (stat("./data", &st) == -1)
    {
        // Directory doesn't exist, try to create it
        if (mkdir("./data", 0755) != 0 && errno != EEXIST)
        {
            return -1; // Failed to create directory
        }
    }

    FILE *f = fopen("./data/group_members.txt", "a");
    if (!f)
        return -1;
    fprintf(f, "%d %d %d\n", member->group_id, member->user_id, member->status);
    fclose(f);
    return 0;
}
