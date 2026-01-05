## Module 2: Group Management - In-Depth Technical Documentation

Module 2 of the File-sharing app is responsible for all group-related operations, allowing users to create, join, manage, and administer groups within the application. This document provides a comprehensive technical explanation of the architecture, socket communication, database operations, and complete workflow for each group management operation.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Socket Communication Protocol](#socket-communication-protocol)
3. [Database Structure](#database-structure)
4. [Session Management](#session-management)
5. [Detailed Operation Workflows](#detailed-operation-workflows)
6. [Error Handling](#error-handling)
7. [Logging System](#logging-system)

---

## Architecture Overview

### System Components

The group management module consists of several key components:

1. **Client Application**: Sends group-related requests via TCP sockets
2. **Server Main Thread**: Accepts connections and spawns worker threads
3. **Worker Threads**: Handle individual client connections concurrently
4. **Request Handler**: Routes messages to appropriate handlers (`process_client_request`)
5. **Group Handlers**: Execute business logic for each group operation
6. **Database Layer**: Manages persistent storage of groups and memberships
7. **Session Manager**: Tracks active user sessions and authentication state

### Thread Model

- **Main Thread**: Listens on port 3636, accepts connections, creates worker threads
- **Worker Threads**: One per client connection, handles all requests for that client
- **Thread Safety**: Session array protected by `pthread_mutex_t session_lock`

---

## Socket Communication Protocol

### Connection Establishment

1. **Server Setup**:

   ```c
   // Server creates socket, binds to port 3636, listens for connections
   int server_sock = socket(AF_INET, SOCK_STREAM, 0);
   bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
   listen(server_sock, 10);
   ```

2. **Client Connection**:

   - Client connects via TCP to `server_ip:3636`
   - Server accepts connection: `accept(server_sock, ...)`
   - Server creates new session via `add_session(sockfd, client_addr)`
   - Server spawns worker thread: `pthread_create(&tid, NULL, client_handler, new_sock)`

3. **Session Initialization**:
   ```c
   Session {
       socket_fd: client_socket_descriptor,
       user_id: -1,              // Not logged in initially
       username: "Guest",
       client_ip: "127.0.0.1",   // Client's IP address
       is_logged_in: 0            // Authentication state
   }
   ```

### Message Protocol

All messages follow a **packet-based protocol** with a fixed header structure:

#### Packet Structure

```c
typedef struct {
    MessageType type;      // Enum: MSG_CREATE_GROUP, MSG_LIST_GROUPS, etc.
    int payload_len;       // Length of payload in bytes
} PacketHeader;
```

#### Message Types (Group Management)

```c
MSG_CREATE_GROUP      // Create a new group
MSG_LIST_GROUPS       // List all available groups
MSG_JOIN_GROUP        // Request to join a group
MSG_LEAVE_GROUP       // Leave a group
MSG_LIST_MEMBERS       // List members of a group
MSG_KICK_MEMBER       // Remove a member (owner only)
MSG_INVITE_MEMBER      // Invite a user to join
MSG_APPROVE_MEMBER     // Approve a join request (owner only)
MSG_SUCCESS            // Operation succeeded
MSG_ERROR              // Operation failed
MSG_LIST_RESPONSE      // Response containing list data
```

### Packet Transmission Flow

#### Sending a Packet (Server → Client)

```c
int send_packet(int sockfd, int type, const void *payload, int payload_len) {
    // 1. Create header
    PacketHeader header;
    header.type = type;
    header.payload_len = payload_len;

    // 2. Send header (8 bytes: 4 bytes type + 4 bytes length)
    send_all(sockfd, &header, sizeof(PacketHeader));

    // 3. Send payload (if any)
    if (payload_len > 0) {
        send_all(sockfd, payload, payload_len);
    }
}
```

**Example**: Sending success message

```
[Header: MSG_SUCCESS, length=14]
[Payload: "Group created"]
```

#### Receiving a Packet (Client → Server)

```c
int recv_packet(int sockfd, int *type, void *payload_buffer) {
    // 1. Receive header first
    PacketHeader header;
    recv_all(sockfd, &header, sizeof(PacketHeader));

    // 2. Extract message type
    *type = header.type;

    // 3. Receive payload if present
    if (header.payload_len > 0) {
        recv_all(sockfd, payload_buffer, header.payload_len);
        payload_buffer[header.payload_len] = '\0'; // Null-terminate
    }

    return header.payload_len;
}
```

### Reliable Data Transmission

The system uses `send_all()` and `recv_all()` functions to ensure complete data transmission:

- **`send_all()`**: Loops until all bytes are sent (handles partial sends)
- **`recv_all()`**: Loops until all bytes are received (handles partial receives)
- **Error Handling**: Returns -1 on error, -2 on connection closed

---

## Database Structure

### File-Based Storage

The system uses simple text files for persistence (no SQL database):

#### 1. Groups Database (`./data/groups.txt`)

**Format**: `group_id group_name owner_id`

**Example**:

```
1 test_group 1
2 project_alpha 2
3 developers 1
```

**Structure**:

```c
typedef struct {
    int group_id;      // Auto-incremented, unique identifier
    char name[64];     // Group name (max 63 chars)
    int owner_id;      // User ID of the group owner
} GroupInfo;
```

#### 2. Group Members Database (`./data/group_members.txt`)

**Format**: `group_id user_id status`

**Status Values**:

- `0` = Pending (join request awaiting approval)
- `1` = Approved (active member)

**Example**:

```
1 1 1        // User 1 is approved member of group 1
1 2 0        // User 2 has pending request for group 1
2 1 1        // User 1 is approved member of group 2
```

**Structure**:

```c
typedef struct {
    int group_id;      // Which group
    int user_id;       // Which user
    int status;        // 0=Pending, 1=Approved
} GroupMemberInfo;
```

### Database Operations

#### Reading Groups

```c
int db_read_groups(GroupInfo *groups, int max_count) {
    // Opens "./data/groups.txt" in read mode
    // Parses each line: group_id name owner_id
    // Returns: count of groups read, or -1 on error
}
```

#### Writing Groups

```c
int db_write_group(const GroupInfo *group) {
    // Opens "./data/groups.txt" in append mode
    // Writes: "group_id name owner_id\n"
    // Creates data directory if it doesn't exist
    // Returns: 0 on success, -1 on error
}
```

#### Reading Members

```c
int db_read_group_members(GroupMemberInfo *members, int max_count) {
    // Opens "./data/group_members.txt" in read mode
    // Parses each line: group_id user_id status
    // Returns: count of memberships read, or -1 on error
}
```

#### Writing Members

```c
int db_write_group_member(const GroupMemberInfo *member) {
    // Opens "./data/group_members.txt" in append mode
    // Writes: "group_id user_id status\n"
    // Returns: 0 on success, -1 on error
}
```

**Note**: For updates (approve, leave, kick), the entire file is rewritten to ensure consistency.

---

## Session Management

### Session Lifecycle

1. **Connection**: Client connects → `add_session()` creates session entry
2. **Authentication**: User logs in → Session updated with `user_id`, `username`, `is_logged_in=1`
3. **Active Use**: All requests use `find_session(sockfd)` to get user context
4. **Disconnection**: Client disconnects → `remove_session()` frees session

### Session Structure

```c
typedef struct {
    int socket_fd;              // Socket descriptor for this connection
    int user_id;                // User ID (-1 if not logged in)
    char username[50];          // Username ("Guest" if not logged in)
    char client_ip[INET_ADDRSTRLEN]; // Client's IP address
    int is_logged_in;           // 0=Guest, 1=Authenticated
} Session;
```

### Thread Safety

- **Mutex Protection**: `pthread_mutex_t session_lock` protects the sessions array
- **Operations**: All session access (add, remove, find) are mutex-protected
- **Concurrent Access**: Multiple threads can safely access sessions simultaneously

---

## Detailed Operation Workflows

### 1. Create Group

**Client Request**:

```
[Header: MSG_CREATE_GROUP, payload_len=9]
[Payload: "MyGroup"]
```

**Server Processing** (`handle_create_group`):

1. **Authentication Check**:

   ```c
   Session *s = find_session(sockfd);
   if (!s || !s->is_logged_in) {
       send_packet(sockfd, MSG_ERROR, "Login required", 14);
       return;
   }
   ```

2. **Group Creation** (`db_create_group`):

   - Read all existing groups from `./data/groups.txt`
   - Find maximum `group_id` in existing groups
   - Calculate new `group_id = max_id + 1`
   - Create `GroupInfo` structure:
     ```c
     GroupInfo new_group = {
         group_id: max_id + 1,
         name: "MyGroup",
         owner_id: s->user_id
     };
     ```
   - Append to `./data/groups.txt`: `"3 MyGroup 1\n"`

3. **Response**:

   - **Success**: `MSG_SUCCESS` with message `"Group 'MyGroup' created with ID: 3"`
   - **Failure**: `MSG_ERROR` with message `"Failed to create group"`

4. **Logging**:
   ```
   [127.0.0.1] User 'admin' (ID 1) created group 'MyGroup' (ID: 3)
   ```

**Socket Flow**:

```
Client → [MSG_CREATE_GROUP + "MyGroup"] → Server
Server → [MSG_SUCCESS + "Group 'MyGroup' created with ID: 3"] → Client
```

---

### 2. List Groups

**Client Request**:

```
[Header: MSG_LIST_GROUPS, payload_len=0]
[No payload]
```

**Server Processing** (`handle_list_groups`):

1. **Read Groups**:

   ```c
   GroupInfo groups[256];
   int count = db_read_groups(groups, 256);
   ```

2. **Build Response**:

   ```c
   char buffer[BUFFER_SIZE] = "--- Available Groups ---\n";
   for (int i = 0; i < count; i++) {
       snprintf(line, "[ID: %d] %s (Owner: %d)\n",
                groups[i].group_id, groups[i].name, groups[i].owner_id);
       strcat(buffer, line);
   }
   ```

3. **Response**:

   - **Success**: `MSG_LIST_RESPONSE` with formatted list
   - **Error**: `MSG_ERROR` with `"Database error"` if read fails

4. **Logging**:
   ```
   [127.0.0.1] User 'admin' (ID 1) listed groups (3 groups found)
   ```

**Socket Flow**:

```
Client → [MSG_LIST_GROUPS] → Server
Server → [MSG_LIST_RESPONSE + "--- Available Groups ---\n[ID: 1] test_group (Owner: 1)\n..."] → Client
```

---

### 3. Join Group

**Client Request**:

```
[Header: MSG_JOIN_GROUP, payload_len=1]
[Payload: "1"]  // group_id
```

**Server Processing** (`handle_join_group`):

1. **Authentication Check**: Verify user is logged in

2. **Parse Group ID**:

   ```c
   int group_id = atoi(payload);  // Convert "1" to integer 1
   ```

3. **Check Existing Membership**:

   ```c
   GroupMemberInfo members[512];
   int count = db_read_group_members(members, 512);
   for (int i = 0; i < count; i++) {
       if (members[i].group_id == group_id &&
           members[i].user_id == s->user_id) {
           // Already a member or pending
           send_packet(sockfd, MSG_ERROR, "Already a member or pending", 27);
           return;
       }
   }
   ```

4. **Create Pending Request**:

   ```c
   GroupMemberInfo new_m = {group_id, s->user_id, 0}; // status=0 (Pending)
   db_write_group_member(&new_m);  // Appends to group_members.txt
   ```

5. **Response**:

   - **Success**: `MSG_SUCCESS` with `"Join request sent to owner"`
   - **Failure**: `MSG_ERROR` with `"Request failed"`

6. **Logging**:
   ```
   [127.0.0.1] User 'user2' (ID 2) requested to join group 1
   ```

**Socket Flow**:

```
Client → [MSG_JOIN_GROUP + "1"] → Server
Server → [MSG_SUCCESS + "Join request sent to owner"] → Client
```

**Database State After Join**:

```
# Before: group_members.txt
1 1 1

# After: group_members.txt
1 1 1
1 2 0    # New pending request
```

---

### 4. Approve Member

**Client Request**:

```
[Header: MSG_APPROVE_MEMBER, payload_len=3]
[Payload: "1 2"]  // "group_id user_id"
```

**Server Processing** (`handle_approve_member`):

1. **Parse Parameters**:

   ```c
   int group_id, target_user_id;
   sscanf(payload, "%d %d", &group_id, &target_user_id);
   ```

2. **Owner Verification**:

   ```c
   GroupInfo groups[256];
   int g_count = db_read_groups(groups, 256);
   int is_owner = 0;
   for (int i = 0; i < g_count; i++) {
       if (groups[i].group_id == group_id &&
           groups[i].owner_id == s->user_id) {
           is_owner = 1;
           break;
       }
   }
   if (!is_owner) {
       send_packet(sockfd, MSG_ERROR, "Only owners can approve", 23);
       return;
   }
   ```

3. **Update Membership Status**:

   ```c
   // Read all memberships
   GroupMemberInfo members[512];
   int count = db_read_group_members(members, 512);

   // Find and update the pending request
   FILE *f = fopen("./data/group_members.txt", "w");  // Overwrite mode
   for (int i = 0; i < count; i++) {
       if (members[i].group_id == group_id &&
           members[i].user_id == target_user_id) {
           members[i].status = 1;  // Change from 0 (Pending) to 1 (Approved)
           found = 1;
       }
       fprintf(f, "%d %d %d\n", members[i].group_id,
               members[i].user_id, members[i].status);
   }
   fclose(f);
   ```

4. **Response**:

   - **Success**: `MSG_SUCCESS` with `"Member approved"`
   - **Not Found**: `MSG_ERROR` with `"Request not found"`

5. **Logging**:
   ```
   [127.0.0.1] User 'admin' (ID 1) approved user 2 to join group 1
   ```

**Socket Flow**:

```
Client → [MSG_APPROVE_MEMBER + "1 2"] → Server
Server → [MSG_SUCCESS + "Member approved"] → Client
```

**Database State After Approval**:

```
# Before: group_members.txt
1 1 1
1 2 0    # Pending

# After: group_members.txt
1 1 1
1 2 1    # Now approved
```

---

### 5. Leave Group

**Client Request**:

```
[Header: MSG_LEAVE_GROUP, payload_len=1]
[Payload: "1"]  // group_id
```

**Server Processing** (`handle_leave_group`):

1. **Owner Check** (Owners cannot leave):

   ```c
   GroupInfo groups[256];
   int g_count = db_read_groups(groups, 256);
   for (int i = 0; i < g_count; i++) {
       if (groups[i].group_id == group_id &&
           groups[i].owner_id == s->user_id) {
           send_packet(sockfd, MSG_ERROR, "Owner cannot leave the group", 28);
           return;
       }
   }
   ```

2. **Remove Membership**:

   ```c
   GroupMemberInfo members[512];
   int count = db_read_group_members(members, 512);
   FILE *f = fopen("./data/group_members.txt", "w");
   for (int i = 0; i < count; i++) {
       // Skip the user's membership (effectively removing it)
       if (members[i].group_id == group_id &&
           members[i].user_id == s->user_id) {
           found = 1;
           continue;  // Don't write this line
       }
       fprintf(f, "%d %d %d\n", members[i].group_id,
               members[i].user_id, members[i].status);
   }
   fclose(f);
   ```

3. **Response**:

   - **Success**: `MSG_SUCCESS` with `"Left group successfully"`
   - **Not Member**: `MSG_ERROR` with `"You are not in this group"`

4. **Logging**:
   ```
   [127.0.0.1] User 'user2' (ID 2) left group 1
   ```

**Socket Flow**:

```
Client → [MSG_LEAVE_GROUP + "1"] → Server
Server → [MSG_SUCCESS + "Left group successfully"] → Client
```

---

### 6. List Members

**Client Request**:

```
[Header: MSG_LIST_MEMBERS, payload_len=1]
[Payload: "1"]  // group_id
```

**Server Processing** (`handle_list_members`):

1. **Read Memberships**:

   ```c
   int group_id = atoi(payload);
   GroupMemberInfo members[512];
   int count = db_read_group_members(members, 512);
   ```

2. **Filter and Format**:

   ```c
   char buffer[BUFFER_SIZE] = "--- Group Members ---\n";
   int member_count = 0;
   for (int i = 0; i < count; i++) {
       if (members[i].group_id == group_id) {
           member_count++;
           snprintf(line, "User ID: %d (%s)\n",
                    members[i].user_id,
                    members[i].status == 1 ? "Member" : "Pending");
           strcat(buffer, line);
       }
   }
   ```

3. **Response**: `MSG_LIST_RESPONSE` with formatted member list

4. **Logging**:
   ```
   [127.0.0.1] User 'admin' (ID 1) listed members of group 1 (2 members)
   ```

**Socket Flow**:

```
Client → [MSG_LIST_MEMBERS + "1"] → Server
Server → [MSG_LIST_RESPONSE + "--- Group Members ---\nUser ID: 1 (Member)\nUser ID: 2 (Pending)\n"] → Client
```

---

### 7. Kick Member

**Client Request**:

```
[Header: MSG_KICK_MEMBER, payload_len=3]
[Payload: "1 2"]  // "group_id user_id"
```

**Server Processing** (`handle_kick_member`):

1. **Owner Verification**: Same as approve member

2. **Remove Member**:

   ```c
   GroupMemberInfo members[512];
   int count = db_read_group_members(members, 512);
   FILE *f = fopen("./data/group_members.txt", "w");
   for (int i = 0; i < count; i++) {
       // Skip the kicked user's membership
       if (members[i].group_id == group_id &&
           members[i].user_id == target_id) {
           continue;  // Don't write this line
       }
       fprintf(f, "%d %d %d\n", members[i].group_id,
               members[i].user_id, members[i].status);
   }
   fclose(f);
   ```

3. **Response**: `MSG_SUCCESS` with `"User kicked"`

4. **Logging**:
   ```
   [127.0.0.1] User 'admin' (ID 1) kicked user 2 from group 1
   ```

**Socket Flow**:

```
Client → [MSG_KICK_MEMBER + "1 2"] → Server
Server → [MSG_SUCCESS + "User kicked"] → Client
```

---

### 8. Invite Member

**Client Request**:

```
[Header: MSG_INVITE_MEMBER, payload_len=3]
[Payload: "1 3"]  // "group_id user_id"
```

**Server Processing** (`handle_invite_member`):

1. **Create Auto-Approved Membership**:

   ```c
   int group_id, target_id;
   sscanf(payload, "%d %d", &group_id, &target_id);

   GroupMemberInfo new_m = {group_id, target_id, 1}; // status=1 (Approved)
   db_write_group_member(&new_m);  // Appends directly as approved
   ```

2. **Response**: `MSG_SUCCESS` with `"User invited and added"`

3. **Logging**:
   ```
   [127.0.0.1] User 'admin' (ID 1) invited user 3 to group 1
   ```

**Socket Flow**:

```
Client → [MSG_INVITE_MEMBER + "1 3"] → Server
Server → [MSG_SUCCESS + "User invited and added"] → Client
```

**Note**: Invite bypasses the approval process - user is immediately added as an approved member.

---

## Error Handling

### Error Categories

1. **Authentication Errors**:

   - User not logged in → `MSG_ERROR: "Login required"`
   - Handled in: `handle_create_group`, `handle_join_group`, `handle_leave_group`

2. **Authorization Errors**:

   - Not group owner → `MSG_ERROR: "Only owners can approve"`
   - Owner trying to leave → `MSG_ERROR: "Owner cannot leave the group"`
   - Handled in: `handle_approve_member`, `handle_kick_member`, `handle_leave_group`

3. **Validation Errors**:

   - Already a member → `MSG_ERROR: "Already a member or pending"`
   - Not a member → `MSG_ERROR: "You are not in this group"`
   - Invalid parameters → `MSG_ERROR: "Usage: APPROVE <group_id> <user_id>"`

4. **Database Errors**:

   - File read failure → `MSG_ERROR: "Database error"`
   - File write failure → `MSG_ERROR: "Failed to create group"` / `"Request failed"`

5. **Not Found Errors**:
   - Join request not found → `MSG_ERROR: "Request not found"`

### Error Response Format

All errors follow the same pattern:

```
[Header: MSG_ERROR, payload_len=N]
[Payload: "Error message string"]
```

---

## Logging System

### Log Format

All group operations are logged to `./data/server.log` with the following format:

```
[YYYY-MM-DD HH:MM:SS] [IP_ADDRESS] User 'username' (ID X) <action description>
```

### Log Examples

```
[2025-01-05 17:30:15] [127.0.0.1] User 'admin' (ID 1) created group 'test' (ID: 1)
[2025-01-05 17:30:20] [127.0.0.1] User 'user2' (ID 2) requested to join group 1
[2025-01-05 17:30:25] [127.0.0.1] User 'admin' (ID 1) approved user 2 to join group 1
[2025-01-05 17:30:30] [127.0.0.1] User 'admin' (ID 1) listed groups (3 groups found)
[2025-01-05 17:30:35] [127.0.0.1] User 'user2' (ID 2) left group 1
[2025-01-05 17:30:40] [127.0.0.1] User 'admin' (ID 1) kicked user 3 from group 1
```

### Logging Function

```c
void log_activity(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");  // Append mode
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    fprintf(f, "[%s] %s\n", time_str, msg);  // Write to file
    printf("[%s] %s\n", time_str, msg);       // Also print to console
    fclose(f);
}
```

### Log Prefix Helper

```c
void get_group_log_prefix(int sockfd, char *buffer) {
    Session *s = find_session(sockfd);
    if (s && s->is_logged_in) {
        snprintf(buffer, 256, "[%s] User '%s' (ID %d)",
                 s->client_ip, s->username, s->user_id);
    } else if (s) {
        snprintf(buffer, 256, "[%s] Guest", s->client_ip);
    } else {
        snprintf(buffer, 256, "[Unknown] Guest");
    }
}
```

---

## Complete Request Flow Diagram

```
┌─────────┐
│ Client  │
└────┬────┘
     │ 1. Connect to server:port
     ├─────────────────────────────┐
     │                             │
┌────▼─────────────────────────────▼────┐
│         Server Main Thread            │
│  - Accepts connection                 │
│  - Creates session                    │
│  - Spawns worker thread              │
└────┬──────────────────────────────────┘
     │
     │ 2. Worker thread created
     │
┌────▼──────────────────────────────────┐
│      Worker Thread (per client)       │
│  - Receives packets via recv_packet() │
│  - Calls process_client_request()     │
└────┬──────────────────────────────────┘
     │
     │ 3. Route to handler
     │
┌────▼──────────────────────────────────┐
│      process_client_request()          │
│  - Switch on message type             │
│  - Call appropriate handler           │
└────┬──────────────────────────────────┘
     │
     ├─► handle_create_group()
     ├─► handle_list_groups()
     ├─► handle_join_group()
     ├─► handle_approve_member()
     ├─► handle_leave_group()
     ├─► handle_list_members()
     ├─► handle_kick_member()
     └─► handle_invite_member()
     │
     │ 4. Handler executes
     │
┌────▼──────────────────────────────────┐
│      Handler Function                  │
│  - find_session(sockfd)              │
│  - Validate authentication            │
│  - db_read_groups() / db_write_*()   │
│  - send_packet() response             │
│  - log_activity()                    │
└────┬──────────────────────────────────┘
     │
     │ 5. Response sent
     │
┌────▼────┐
│ Client  │
└─────────┘
```

---

## Summary

Module 2 provides a complete group management system with:

- **TCP Socket Communication**: Reliable packet-based protocol with header + payload
- **Thread-Safe Session Management**: Mutex-protected session array for concurrent access
- **File-Based Persistence**: Simple text files for groups and memberships
- **Comprehensive Logging**: All operations logged with timestamps and user context
- **Robust Error Handling**: Clear error messages for all failure scenarios
- **Access Control**: Owner-only operations (approve, kick) with proper validation

The architecture supports multiple concurrent clients, each handled by a dedicated worker thread, ensuring scalability and isolation between user sessions.
