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
int remove_directory_recursive(const char *path); // From handle_file.c

// Helper function to get log prefix with user info
void get_group_log_prefix(int sockfd, char *buffer)
{
    Session *s = find_session(sockfd);
    if (s && s->is_logged_in)
    {
        snprintf(buffer, 256, "[%s] User '%s' (ID %d)", s->client_ip, s->username, s->user_id);
    }
    else if (s)
    {
        snprintf(buffer, 256, "[%s] Guest", s->client_ip);
    }
    else
    {
        snprintf(buffer, 256, "[Unknown] Guest");
    }
}

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
    // snprintf(dir_path, sizeof(dir_path), "./data/Group_%d", group_id);
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
        // Auto-add owner as an approved member (status 1)
        GroupMemberInfo owner_membership = {group_id, owner_id, 1};
        db_write_group_member(&owner_membership);
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

        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s created group '%s' (ID: %d)", log_prefix, payload, group_id);
        log_activity(log_msg);

        int dir_res = db_create_group_directory(group_id);
        if (dir_res < 0)
        {
            char log_prefix[256];
            get_group_log_prefix(sockfd, log_prefix);
            char log_msg[512];
            snprintf(log_msg, sizeof(log_msg), "%s failed to create directory for group %d", log_prefix, group_id);
            log_activity(log_msg);
            // Optionally notify the client (non-fatal)
            send_packet(sockfd, MSG_ERROR, "Warning: directory not created", 29);
        }
        else
        {
            char log_prefix[256];
            get_group_log_prefix(sockfd, log_prefix);
            snprintf(log_msg, sizeof(log_msg), "%s successfully created directory for group %d", log_prefix, group_id);
            log_activity(log_msg);
        }
    }
    else
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s failed to create group '%s'", log_prefix, payload);
        log_activity(log_msg);
        send_packet(sockfd, MSG_ERROR, "Failed to create group", 22);
    }
}

void handle_list_groups(int sockfd)
{
    // Session *s = find_session(sockfd);
    GroupInfo groups[256];
    int count = db_read_groups(groups, 256);

    if (count < 0)
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s failed to list groups (database error)", log_prefix);
        log_activity(log_msg);
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

    char log_prefix[256];
    get_group_log_prefix(sockfd, log_prefix);
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "%s listed groups (%d groups found)", log_prefix, count);
    log_activity(log_msg);
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
            char log_prefix[256];
            get_group_log_prefix(sockfd, log_prefix);
            char log_msg[512];
            snprintf(log_msg, sizeof(log_msg), "%s attempted to join group %d (already a member or pending)", log_prefix, group_id);
            log_activity(log_msg);
            send_packet(sockfd, MSG_ERROR, "Already a member or pending", 27);
            return;
        }
    }

    GroupMemberInfo new_m = {group_id, s->user_id, 0}; // 0 = Pending
    if (db_write_group_member(&new_m) == 0)
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s requested to join group %d", log_prefix, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_SUCCESS, "Join request sent to owner", 26);
    }
    else
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s failed to request join group %d", log_prefix, group_id);
        log_activity(log_msg);
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
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s attempted to approve member in group %d (not owner)", log_prefix, group_id);
        log_activity(log_msg);
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
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s approved user %d to join group %d", log_prefix, target_user_id, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_SUCCESS, "Member approved", 15);
    }
    else
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s attempted to approve user %d in group %d (request not found)", log_prefix, target_user_id, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_ERROR, "Request not found", 17);
    }
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
            char log_prefix[256];
            get_group_log_prefix(sockfd, log_prefix);
            char log_msg[512];
            snprintf(log_msg, sizeof(log_msg), "%s attempted to leave group %d (owner cannot leave)", log_prefix, group_id);
            log_activity(log_msg);
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
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s left group %d", log_prefix, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_SUCCESS, "Left group successfully", 23);
    }
    else
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s attempted to leave group %d (not a member)", log_prefix, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_ERROR, "You are not in this group", 25);
    }
}

void handle_list_members(int sockfd, char *payload)
{
    int group_id = atoi(payload);
    GroupMemberInfo members[512];
    int count = db_read_group_members(members, 512);

    int member_count = 0;
    char buffer[BUFFER_SIZE] = "--- Group Members ---\n";
    for (int i = 0; i < count; i++)
    {
        if (members[i].group_id == group_id)
        {
            member_count++;
            char line[64];
            snprintf(line, sizeof(line), "User ID: %d (%s)\n",
                     members[i].user_id, members[i].status == 1 ? "Member" : "Pending");
            strcat(buffer, line);
        }
    }
    send_packet(sockfd, MSG_LIST_RESPONSE, buffer, strlen(buffer));

    char log_prefix[256];
    get_group_log_prefix(sockfd, log_prefix);
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "%s listed members of group %d (%d members)", log_prefix, group_id, member_count);
    log_activity(log_msg);
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
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s attempted to kick user %d from group %d (not owner)", log_prefix, target_id, group_id);
        log_activity(log_msg);
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

    char log_prefix[256];
    get_group_log_prefix(sockfd, log_prefix);
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "%s kicked user %d from group %d", log_prefix, target_id, group_id);
    log_activity(log_msg);
    send_packet(sockfd, MSG_SUCCESS, "User kicked", 11);
}

void handle_invite_member(int sockfd, char *payload)
{
    Session *s = find_session(sockfd);
    int group_id, target_id;
    sscanf(payload, "%d %d", &group_id, &target_id);

    // 1. Check group owner and requester membership
    GroupInfo groups[256];
    int g_count = db_read_groups(groups, 256);
    int is_owner = 0;

    for (int i = 0; i < g_count; i++)
    {
        if (groups[i].group_id == group_id)
        {
            if (groups[i].owner_id == s->user_id)
                is_owner = 1;
            break;
        }
    }

    GroupMemberInfo members[512];
    int m_count = db_read_group_members(members, 512);
    int is_member = 0;
    for (int i = 0; i < m_count; i++)
    {
        if (members[i].group_id == group_id && members[i].user_id == s->user_id && members[i].status == 1)
        {
            is_member = 1;
            break;
        }
    }

    if (!is_owner && !is_member)
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s attempted to invite user %d to group %d (not owner nor member)", log_prefix, target_id, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_ERROR, "Only group members or owner can invite", 37);
        return;
    }

    GroupMemberInfo new_m = {group_id, target_id, is_owner ? 1 : 0}; // 1=Approved, 0=Pending

    if (db_write_group_member(&new_m) == 0)
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        if (is_owner)
        {
            snprintf(log_msg, sizeof(log_msg), "%s (owner) invited and added user %d to group %d", log_prefix, target_id, group_id);
            log_activity(log_msg);
            send_packet(sockfd, MSG_SUCCESS, "User invited and added", 22);
        }
        else if (is_member)
        {
            snprintf(log_msg, sizeof(log_msg), "%s (member) invited user %d to group %d (pending approval)", log_prefix, target_id, group_id);
            log_activity(log_msg);
            send_packet(sockfd, MSG_SUCCESS, "User invited, pending approval", 28);
        }
    }
    else
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s failed to invite user %d to group %d", log_prefix, target_id, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_ERROR, "Invite failed", 13);
    }
}

void handle_delete_group(int sockfd, char *payload)
{
    Session *s = find_session(sockfd);
    if (!s || !s->is_logged_in)
    {
        send_packet(sockfd, MSG_ERROR, "Login required", 14);
        return;
    }

    int group_id = atoi(payload);
    if (group_id <= 0)
    {
        send_packet(sockfd, MSG_ERROR, "Invalid group ID", 16);
        return;
    }

    // 1. Check if user is the owner of this group
    GroupInfo groups[256];
    int g_count = db_read_groups(groups, 256);
    int is_owner = 0;
    int group_exists = 0;
    char group_name[64] = "";

    for (int i = 0; i < g_count; i++)
    {
        if (groups[i].group_id == group_id)
        {
            group_exists = 1;
            strncpy(group_name, groups[i].name, 63);
            if (groups[i].owner_id == s->user_id)
            {
                is_owner = 1;
            }
            break;
        }
    }

    if (!group_exists)
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s attempted to delete group %d (group not found)", log_prefix, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_ERROR, "Group not found", 15);
        return;
    }

    if (!is_owner)
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s attempted to delete group %d (not owner)", log_prefix, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_ERROR, "Only owner can delete the group", 30);
        return;
    }

    // 2. Remove all members from group_members.txt
    GroupMemberInfo members[512];
    int m_count = db_read_group_members(members, 512);
    FILE *f_members = fopen("./data/group_members.txt", "w");
    if (!f_members)
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s failed to delete group %d (cannot open group_members.txt)", log_prefix, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_ERROR, "Failed to delete group (database error)", 36);
        return;
    }
    for (int i = 0; i < m_count; i++)
    {
        if (members[i].group_id != group_id)
        {
            fprintf(f_members, "%d %d %d\n", members[i].group_id, members[i].user_id, members[i].status);
        }
    }
    fclose(f_members);

    // 3. Remove the group from groups.txt
    FILE *f_groups = fopen("./data/groups.txt", "w");
    if (!f_groups)
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s failed to delete group %d (cannot open groups.txt)", log_prefix, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_ERROR, "Failed to delete group (database error)", 36);
        return;
    }
    for (int i = 0; i < g_count; i++)
    {
        if (groups[i].group_id != group_id)
        {
            fprintf(f_groups, "%d %s %d\n", groups[i].group_id, groups[i].name, groups[i].owner_id);
        }
    }
    fclose(f_groups);

    // 4. Delete the group directory
    char dir_path[256];
    snprintf(dir_path, sizeof(dir_path), "./data/Group_%d", group_id);
    errno = 0; // Clear errno before call
    int dir_res = remove_directory_recursive(dir_path);
    int saved_errno = errno; // Save errno immediately after call

    if (dir_res == 0 || saved_errno == ENOENT) // ENOENT means directory doesn't exist, which is fine
    {
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s deleted group '%s' (ID: %d)", log_prefix, group_name, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_SUCCESS, "Group deleted successfully", 25);
    }
    else
    {
        // Group and members are already removed from DB, but directory deletion failed
        // Log warning but still report success since DB cleanup succeeded
        char log_prefix[256];
        get_group_log_prefix(sockfd, log_prefix);
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "%s deleted group '%s' (ID: %d) but directory deletion failed", log_prefix, group_name, group_id);
        log_activity(log_msg);
        send_packet(sockfd, MSG_SUCCESS, "Group deleted (directory cleanup warning)", 42);
    }
}