#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include "common.h"
#include "protocol.h"
#include "network.h"
#include "db.h"

// Khai báo hàm để hàm bên dưới gọi
Session *find_session(int sockfd);
void log_activity(const char *msg);

// DB Helpers from db.c
// int db_read_groups(GroupInfo *groups, int max_count);
// int db_write_group(const GroupInfo *group);
// int db_read_group_members(GroupMemberInfo *members, int max_count);
// int db_write_group_member(const GroupMemberInfo *member);

// --- DB-LEVEL LOGIC ---

int db_create_group_directory(int group_id)
{
    char dir_path[256];
    // Server runs from project root, so use "./data/files/Group_X"
    snprintf(dir_path, sizeof(dir_path), "./data/files/Group_%d", group_id);

    int res = mkdir(dir_path, 0755);
    return (res == 0 || errno == EEXIST) ? 0 : -1;
}

int db_create_group(const char *group_name, int owner_id)
{
    GroupInfo groups[256];
    int count = db_read_groups(groups, 256);
    int max_id = 0;
    for (int i = 0; i < count; ++i)
    {
        if (groups[i].group_id > max_id)
            max_id = groups[i].group_id;
    }
    int group_id = max_id + 1;

    GroupInfo new_group = {group_id, "", owner_id};
    strncpy(new_group.name, group_name, 63);

    if (db_write_group(&new_group) == 0)
    {
        int dir_res = db_create_group_directory(group_id);

        // Auto-add owner as an approved member (status 1)
        GroupMemberInfo owner_membership = {group_id, owner_id, 1};
        int mem_res = db_write_group_member(&owner_membership);
        return group_id;
    }
    return -1;
}

// --- MODULE 2 HANDLERS ---

void handle_create_group(int sockfd, char *payload)
{
    Session *s = find_session(sockfd);
    if (!s || !s->is_logged_in)
    {
        send_packet(sockfd, MSG_ERROR, "Login required", 14);
        return;
    }
    int group_id = db_create_group(payload, s->user_id);
    if (group_id > 0)
    {
        char msg[128];
        snprintf(msg, sizeof(msg), "Group '%s' created with ID: %d", payload, group_id);
        send_packet(sockfd, MSG_SUCCESS, msg, strlen(msg));

        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "User %s created group %d", s->username, group_id);
        log_activity(log_msg);
    }
    else
    {
        send_packet(sockfd, MSG_ERROR, "Failed to create group", 22);
    }
}

void handle_list_groups(int sockfd)
{
    GroupInfo groups[256];
    int count = db_read_groups(groups, 256);

    if (count < 0)
    {
        send_packet(sockfd, MSG_ERROR, "Database error", 14);
        return;
    }

    char buffer[BUFFER_SIZE] = "--- Available Groups ---\n";
    for (int i = 0; i < count; i++)
    {
        char line[128];
        snprintf(line, sizeof(line), "[ID: %d] %s (Owner: %d)\n", groups[i].group_id, groups[i].name, groups[i].owner_id);
        if (strlen(buffer) + strlen(line) < BUFFER_SIZE - 1)
            strcat(buffer, line);
    }
    send_packet(sockfd, MSG_LIST_RESPONSE, buffer, strlen(buffer));
}

void handle_join_group(int sockfd, char *payload)
{
    Session *s = find_session(sockfd);
    if (!s || !s->is_logged_in)
    {
        send_packet(sockfd, MSG_ERROR, "Login required", 14);
        return;
    }

    int group_id = atoi(payload);
    GroupMemberInfo members[512];
    int count = db_read_group_members(members, 512);

    for (int i = 0; i < count; i++)
    {
        if (members[i].group_id == group_id && members[i].user_id == s->user_id)
        {
            send_packet(sockfd, MSG_ERROR, "Already a member or pending", 27);
            return;
        }
    }

    GroupMemberInfo new_m = {group_id, s->user_id, 0}; // 0 = Pending
    if (db_write_group_member(&new_m) == 0)
    {
        send_packet(sockfd, MSG_SUCCESS, "Join request sent to owner", 26);
    }
    else
    {
        send_packet(sockfd, MSG_ERROR, "Request failed", 14);
    }
}

void handle_approve_member(int sockfd, char *payload)
{
    int group_id, target_user_id;
    if (sscanf(payload, "%d %d", &group_id, &target_user_id) < 2)
    {
        send_packet(sockfd, MSG_ERROR, "Usage: APPROVE <group_id> <user_id>", 35);
        return;
    }

    Session *s = find_session(sockfd);
    GroupInfo groups[256];
    int g_count = db_read_groups(groups, 256);
    int is_owner = 0;
    for (int i = 0; i < g_count; i++)
    {
        if (groups[i].group_id == group_id && groups[i].owner_id == s->user_id)
            is_owner = 1;
    }

    if (!is_owner)
    {
        send_packet(sockfd, MSG_ERROR, "Only owners can approve", 23);
        return;
    }

    GroupMemberInfo members[512];
    int count = db_read_group_members(members, 512);
    int found = 0;

    // Server runs from project root, so use "./data/group_members.txt"
    FILE *f = fopen("./data/group_members.txt", "w");
    for (int i = 0; i < count; i++)
    {
        if (members[i].group_id == group_id && members[i].user_id == target_user_id)
        {
            members[i].status = 1;
            found = 1;
        }
        fprintf(f, "%d %d %d\n", members[i].group_id, members[i].user_id, members[i].status);
    }
    fclose(f);

    if (found)
        send_packet(sockfd, MSG_SUCCESS, "Member approved", 15);
    else
        send_packet(sockfd, MSG_ERROR, "Request not found", 17);
}

// --- MISSING FUNCTIONS IMPLEMENTED BELOW ---

void handle_leave_group(int sockfd, char *payload)
{
    Session *s = find_session(sockfd);
    int group_id = atoi(payload);

    // Check if user is the owner of this group
    GroupInfo groups[256];
    int g_count = db_read_groups(groups, 256);
    for (int i = 0; i < g_count; i++)
    {
        if (groups[i].group_id == group_id && groups[i].owner_id == s->user_id)
        {
            send_packet(sockfd, MSG_ERROR, "Owner cannot leave the group", 28);
            return;
        }
    }

    GroupMemberInfo members[512];
    int count = db_read_group_members(members, 512);
    int found = 0;

    FILE *f = fopen("./data/group_members.txt", "w");
    for (int i = 0; i < count; i++)
    {
        if (members[i].group_id == group_id && members[i].user_id == s->user_id)
        {
            found = 1;
            continue; // Skip writing this line to "remove" them
        }
        fprintf(f, "%d %d %d\n", members[i].group_id, members[i].user_id, members[i].status);
    }
    fclose(f);

    if (found)
        send_packet(sockfd, MSG_SUCCESS, "Left group successfully", 23);
    else
        send_packet(sockfd, MSG_ERROR, "You are not in this group", 25);
}

void handle_list_members(int sockfd, char *payload)
{
    int group_id = atoi(payload);
    GroupMemberInfo members[512];
    int count = db_read_group_members(members, 512);

    char buffer[BUFFER_SIZE] = "--- Group Members ---\n";
    for (int i = 0; i < count; i++)
    {
        if (members[i].group_id == group_id)
        {
            char line[64];
            snprintf(line, sizeof(line), "User ID: %d (%s)\n",
                     members[i].user_id, members[i].status == 1 ? "Member" : "Pending");
            strcat(buffer, line);
        }
    }
    send_packet(sockfd, MSG_LIST_RESPONSE, buffer, strlen(buffer));
}

void handle_kick_member(int sockfd, char *payload)
{
    int group_id, target_id;
    sscanf(payload, "%d %d", &group_id, &target_id);

    Session *s = find_session(sockfd);
    // 1. Check if requester is owner
    GroupInfo groups[256];
    int g_count = db_read_groups(groups, 256);
    int is_owner = 0;
    for (int i = 0; i < g_count; i++)
    {
        if (groups[i].group_id == group_id && groups[i].owner_id == s->user_id)
            is_owner = 1;
    }

    if (!is_owner)
    {
        send_packet(sockfd, MSG_ERROR, "Only owners can kick members", 27);
        return;
    }

    // 2. Rewrite members file without the kicked user
    GroupMemberInfo members[512];
    int count = db_read_group_members(members, 512);
    FILE *f = fopen("./data/group_members.txt", "w");
    for (int i = 0; i < count; i++)
    {
        if (members[i].group_id == group_id && members[i].user_id == target_id)
            continue;
        fprintf(f, "%d %d %d\n", members[i].group_id, members[i].user_id, members[i].status);
    }
    fclose(f);
    send_packet(sockfd, MSG_SUCCESS, "User kicked", 11);
}

void handle_invite_member(int sockfd, char *payload)
{
    // For simplicity, invite works like an auto-approved join request
    int group_id, target_id;
    sscanf(payload, "%d %d", &group_id, &target_id);

    GroupMemberInfo new_m = {group_id, target_id, 1}; // 1 = Approved
    if (db_write_group_member(&new_m) == 0)
    {
        send_packet(sockfd, MSG_SUCCESS, "User invited and added", 22);
    }
    else
    {
        send_packet(sockfd, MSG_ERROR, "Invite failed", 13);
    }
}