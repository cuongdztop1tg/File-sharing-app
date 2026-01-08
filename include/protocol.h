#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef enum
{
    // System & Status
    MSG_CONNECT,
    MSG_DISCONNECT,
    MSG_SUCCESS,
    MSG_ERROR,

    // Authentication
    MSG_REGISTER,
    MSG_LOGIN,
    MSG_LOGOUT,
    MSG_CHANGE_PASS,
    MSG_DELETE_ACCOUNT,

    // Group management
    MSG_CREATE_GROUP,
    MSG_LIST_GROUPS,
    MSG_JOIN_GROUP,
    MSG_LEAVE_GROUP,
    MSG_LIST_MEMBERS,
    MSG_KICK_MEMBER,
    MSG_INVITE_MEMBER,
    MSG_APPROVE_MEMBER,
    MSG_DELETE_GROUP,

    // File system
    // MSG_LIST_FILES,
    MSG_CREATE_FOLDER,
    MSG_DELETE_ITEM,
    MSG_RENAME_ITEM,
    MSG_MOVE_ITEM,
    MSG_COPY_ITEM,

    // File Transfer
    MSG_UPLOAD_REQ,
    MSG_DOWNLOAD_REQ,
    MSG_FILE_DATA,
    MSG_FILE_END,
    MSG_FILE_ERROR,

    // Directory Listing
    MSG_LIST_FILES,
    MSG_LIST_RESPONSE
} MessageType;

typedef struct
{
    MessageType type;
    int payload_len;
} PacketHeader;

#define BUFFER_SIZE 4096

#endif