#include <stdio.h>
#include "common.h"

/**
 * @brief Prints the main menu commands.
 */
void print_main_menu()
{
    printf("\n====================================\n");
    printf("      FILE SHARING APPLICATION      \n");
    printf("====================================\n");
    printf("1. Register Account:\n");
    printf("   Command: REGISTER <username> <password>\n");
    printf("   Example: REGISTER user1 123456\n");
    printf("\n");
    printf("2. Login:\n");
    printf("   Command: LOGIN <username> <password>\n");
    printf("   Example: LOGIN user1 123456\n");
    printf("\n");
    printf("3. Create Group:\n");
    printf("   Command: CREATE_GROUP <group_name>\n");
    printf("\n");
    printf("4. Exit Application:\n");
    printf("   Command: EXIT\n");
    printf("====================================\n");
}

/**
 * @brief Displays a help message.
 */
void print_help()
{
    print_main_menu();
}

void print_logged_in_menu()
{
    printf("\n====================================\n");
    printf("      FILE SHARING APPLICATION      \n");
    printf("======== USER MAIN MENU ============\n");
    printf("1. List Groups:\n");
    printf("   Command: LIST_GROUPS\n");
    printf("\n");
    printf("2. Create Group:\n");
    printf("   Command: CREATE_GROUP <group_name>\n");
    printf("\n");
    printf("3. Join Group:\n");
    printf("   Command: JOIN_GROUP <group_id>\n");
    printf("\n");
    printf("4. Leave Group:\n");
    printf("   Command: LEAVE_GROUP <group_id>\n");
    printf("\n");
    printf("5. List Members of Group:\n");
    printf("   Command: LIST_MEMBERS <group_id>\n");
    printf("\n");
    printf("6. Kick Member (group owner):\n");
    printf("   Command: KICK_MEMBER <group_id> <username>\n");
    printf("\n");
    printf("7. Invite Member (group owner):\n");
    printf("   Command: INVITE_MEMBER <group_id> <username>\n");
    printf("\n");
    printf("8. Approve Member (group owner):\n");
    printf("   Command: APPROVE_MEMBER <group_id> <username>\n");
    printf("\n");
    printf("9. List Files/Folders in Group:\n");
    printf("   Command: LIST_FILES <group_id> [subfolder]\n");
    printf("\n");
    printf("10. Upload File to Group:\n");
    printf("    Command: UPLOAD <group_id> <local_filename>\n");
    printf("\n");
    printf("11. Download File from Group:\n");
    printf("    Command: DOWNLOAD <group_id> <remote_filename>\n");
    printf("\n");
    printf("12. Create Folder in Group:\n");
    printf("    Command: CREATE_FOLDER <group_id> <foldername>\n");
    printf("\n");
    printf("13. Delete File/Folder in Group:\n");
    printf("    Command: DELETE_ITEM <group_id> <itemname>\n");
    printf("\n");
    printf("14. Rename File/Folder in Group:\n");
    printf("    Command: RENAME_ITEM <group_id> <oldname> <newname>\n");
    printf("\n");
    printf("15. Move File/Folder in Group:\n");
    printf("    Command: MOVE_ITEM <group_id> <itemname> <destination>\n");
    printf("\n");
    printf("16. Copy File/Folder in Group:\n");
    printf("    Command: COPY_ITEM <group_id> <itemname> <destination>\n");
    printf("\n");
    printf("17. Logout:\n");
    printf("    Command: LOGOUT\n");
    printf("\n");
    printf("18. Exit Application:\n");
    printf("    Command: EXIT\n");
    printf("====================================\n");
}
