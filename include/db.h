#ifndef DB_H
#define DB_H

#include "common.h"

// --- GROUP DATA STRUCTURES ---

typedef struct {
    int group_id;
    char name[64];
    int owner_id;
} GroupInfo;

typedef struct {
    int group_id;
    int user_id;
    int status; // 0: Pending, 1: Accepted
} GroupMemberInfo;

// --- USER MANAGEMENT FUNCTIONS ---
int db_check_login(const char *username, const char *password);
int db_register_user(const char *username, const char *password);

// --- GROUP MANAGEMENT FUNCTIONS ---
int db_read_groups(GroupInfo *groups, int max_count);
int db_write_group(const GroupInfo *group);
int db_read_group_members(GroupMemberInfo *members, int max_count);
int db_write_group_member(const GroupMemberInfo *member);

#endif
