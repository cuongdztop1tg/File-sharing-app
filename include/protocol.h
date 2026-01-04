#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef enum {
    // System & Status
    MSG_CONNECT,
    MSG_DISCONNECT,
    MSG_SUCCESS,
    MSG_ERROR,

    // Authentication
    MSG_REGISTER,
    MSG_LOGIN,
    MSG_LOGOUT,
    MSG_CHANGE_PASS, // Payload: "old_password new_password"
    MSG_DELETE_ACCOUNT, // Payload: "password_confirm"

    // Group management
    MSG_CREATE_GROUP,
    MSG_LIST_GROUPS,
    MSG_JOIN_GROUP,
    MSG_LEAVE_GROUP,
    MSG_LIST_MEMBERS,
    MSG_KICK_MEMBER,
    MSG_INVITE_MEMBER,
    MSG_APPROVE_MEMBER,

    // File system
    // MSG_LIST_FILES,
    MSG_CREATE_FOLDER,
    MSG_DELETE_ITEM,
    MSG_RENAME_ITEM,
    MSG_MOVE_ITEM,
    MSG_COPY_ITEM,

    // File Transfer
    MSG_UPLOAD_REQ,     // Yêu cầu upload: Payload = "filename filesize"
    MSG_DOWNLOAD_REQ,   // Yêu cầu download: Payload = "filename"
    MSG_FILE_DATA,      // Gửi 1 gói dữ liệu (Chunk)
    MSG_FILE_END,       // Thông báo đã gửi xong file (EOF)
    MSG_FILE_ERROR,     // Lỗi trong quá trình truyền file
    
    
    // Directory Listing
    MSG_LIST_FILES,     // Client hỏi danh sách file
    MSG_LIST_RESPONSE   // Server trả về danh sách file (chuỗi dài)
} MessageType;

typedef struct {
    MessageType type;
    int payload_len;
} PacketHeader;

#define BUFFER_SIZE 4096

#endif