## Module 2: Group Management

Module 2 of the File-sharing app is responsible for all group-related operations, allowing users to create, join, manage, and administer groups within the application. This is crucial for organizing users into collaborative units, each with their own set of permissions and shared resources.

### Main Functionalities

**1. Create Group**

- **Function:** `handle_create_group(int sockfd, char *payload)`
- **Description:** Allows a user to create a new group. The `payload` typically contains the group name and potentially initial settings (like description, privacy, etc.).
- **Flow:**
  - Validate if the group name is unique.
  - Insert the new group into the database.
  - Assign the creating user as the group owner or admin.

**2. List Groups**

- **Function:** `handle_list_groups(int sockfd)`
- **Description:** Retrieves a list of all available groups or those visible/accessible to the user.
- **Flow:**
  - Query database for groups.
  - Send back the group list to the client.

**3. Join Group**

- **Function:** `handle_join_group(int sockfd, char *payload)`
- **Description:** Allows a user to request to join an existing group. The `payload` contains the group identifier.
- **Flow:**
  - Add the user to a pending/join request list if approval is required; otherwise, add directly to group members.
  - Notify group admins for approval if necessary.

**4. Leave Group**

- **Function:** `handle_leave_group(int sockfd, char *payload)`
- **Description:** Enables a user to leave a group they're currently a member of.
- **Flow:**
  - Remove the user from the group’s member list.
  - Update group data accordingly.

**5. List Members**

- **Function:** `handle_list_members(int sockfd, char *payload)`
- **Description:** Returns the full list of members in a specified group.
- **Flow:**
  - Lookup group based on payload.
  - Retrieve and send the list of members to the requester.

**6. Kick Member**

- **Function:** `handle_kick_member(int sockfd, char *payload)`
- **Description:** Allows a group admin to remove a user from the group.
- **Flow:**
  - Check if requesting user is an admin.
  - Remove the specified member from the group.

**7. Invite Member**

- **Function:** `handle_invite_member(int sockfd, char *payload)`
- **Description:** Enables a member (usually with permissions) to invite others to join a group.
- **Flow:**
  - Send an invitation to the target user.

**8. Approve Member**

- **Function:** `handle_approve_member(int sockfd, char *payload)`
- **Description:** Approves a user's join request.
- **Flow:**
  - Admin reviews pending requests.
  - Approves or rejects users for group membership.

### How Does Group Management Work?

- **Request Handling:**  
  All group-related requests are processed centrally in the function `process_client_request`, which dispatches based on the message type (see `MSG_CREATE_GROUP`, `MSG_LIST_GROUPS`, etc.).
- **Access Control:**  
  Many actions are permission-bound. For example, kicking or approving members usually requires admin status.
- **Persistence:**  
  All group structures and relationships are stored in the backend database, ensuring data persistence and consistency.

### Example Workflow

1. **Creating a Group:**  
   A user sends a "create group" request. If successful, a new group is created, and the user is set as the first admin.

2. **Joining a Group:**  
   A user browses the list of groups and chooses to join one. If approval is needed, they are added to a pending list for admin review.

3. **Member Management:**  
   Admins can invite, approve, or remove members to keep the group organized and secure.

### Summary

Module 2 abstracts all the complexity involved in group-centric collaboration—providing robust endpoints for users and admins to efficiently manage group life cycles within the app.

---
