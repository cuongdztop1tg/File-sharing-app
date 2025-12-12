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
    MSG_LIST_FILES,
    MSG_CREATE_FOLDER,
    MSG_DELETE_ITEM,
    MSG_RENAME_ITEM,
    MSG_MOVE_ITEM,
    MSG_COPY_ITEM,

    // File transfer
    MSG_UPLOAD_REQ,
    MSG_DOWNLOAD_REQ,
    MSG_DILE_DATA
} MessageType;

typedef struct {
    MessageType type;
    int payload_len;
} PacketHeader;