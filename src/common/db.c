#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include "common.h"

pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- USER MANAGEMENT ---

/**
 * @brief Checks if username and password match a record in users.txt
 * @return UserID if success, -1 if failure.
 */
int db_check_login(const char *username, const char *password)
{
    pthread_mutex_lock(&db_mutex);

    FILE *f = fopen(USER_DB_FILE, "r");
    if (!f)
    {
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }
    int id;
    char u[50], p[50];

    // Format in file: ID Username Password
    while (fscanf(f, "%d %s %s", &id, u, p) != EOF)
    {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0)
        {
            fclose(f);
            pthread_mutex_unlock(&db_mutex);
            return id;                       // Login success
        }
    }

    fclose(f);
    pthread_mutex_unlock(&db_mutex);
    return -1;                       // Login failed
}

/**
 * @brief Registers a new user.
 * @return New UserID if success, -1 if username already exists.
 */
int db_register_user(const char *username, const char *password)
{
    pthread_mutex_lock(&db_mutex);
    FILE *f = fopen(USER_DB_FILE, "a+"); // Read + Append
    if (!f)
    {
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }
    int id = 0, max_id = 0;
    char u[50], p[50];
    int exists = 0;

    // Check for duplicates and find the highest ID
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
        pthread_mutex_unlock(&db_mutex);
        return -1; // Error: User exists
    }

    // Append new user
    int new_id = max_id + 1;
    fseek(f, 0, SEEK_END);
    fprintf(f, "%d %s %s\n", new_id, username, password);

    fclose(f);
    pthread_mutex_unlock(&db_mutex);
    return new_id;
}

/**
 * @brief Change password for user
 * @return 0 if success, -1 if fail
 */
int db_change_password(int user_id, const char *new_password)
{
    pthread_mutex_lock(&db_mutex);
    FILE *f = fopen(USER_DB_FILE, "r");
    FILE *temp = fopen("users.tmp", "w");

    if (!f || !temp)
    {
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    int id;
    char u[50], p[50];
    int found = 0;

    while (fscanf(f, "%d %s %s", &id, u, p) != EOF)
    {
        if (id == user_id)
        {
            fprintf(temp, "%d %s %s\n", id, u, new_password);
            found = 1;
        }
        else
        {
            fprintf(temp, "%d %s %s\n", id, u, p);
        }
    }

    fclose(f);
    fclose(temp);

    if (found)
    {
        remove(USER_DB_FILE); 
        rename("users.tmp", USER_DB_FILE);
        pthread_mutex_unlock(&db_mutex);
        return 0;
    }
    else
    {
        remove("users.tmp");
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }
}

/**
 * @brief Delete user from database
 */
int db_delete_user(int user_id)
{
    pthread_mutex_lock(&db_mutex);
    FILE *f = fopen(USER_DB_FILE, "r");
    FILE *temp = fopen("users.tmp", "w");
    if (!f || !temp)
    {
        if (f)
            fclose(f);
        if (temp)
            fclose(temp);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    int id;
    char u[50], p[50];
    int found = 0;

    while (fscanf(f, "%d %s %s", &id, u, p) != EOF)
    {
        if (id == user_id)
        {
            found = 1;
        }
        else
        {
            fprintf(temp, "%d %s %s\n", id, u, p);
        }
    }

    fclose(f);
    fclose(temp);

    if (found)
    {
        remove(USER_DB_FILE);
        rename("users.tmp", USER_DB_FILE);
        pthread_mutex_unlock(&db_mutex);
        return 0;
    }
    else
    {
        remove("users.tmp");
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }
}

//--------- GROUP MANAGEMENT ---------

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
