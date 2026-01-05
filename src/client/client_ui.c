#include <stdio.h>
#include "common.h"

/**
 * @brief Prints the main menu commands (before login).
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
    printf("3. Exit Application:\n");
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

/**
 * @brief Prints menu after user logs in (matches actual implementation).
 */
void print_logged_in_menu()
{
    printf("\n====================================\n");
    printf("      FILE SHARING APPLICATION      \n");
    printf("======== USER MAIN MENU ============\n");
    printf("\n--- GROUP MANAGEMENT ---\n");
    printf("1. Create Group:\n");
    printf("   Command: CREATE_GROUP <group_name>\n");
    printf("   Example: CREATE_GROUP MyProject\n");
    printf("\n");
    printf("2. List All Groups:\n");
    printf("   Command: LIST_GROUPS\n");
    printf("\n");
    printf("3. Join Group:\n");
    printf("   Command: JOIN_GROUP <group_id>\n");
    printf("   Example: JOIN_GROUP 1\n");
    printf("\n");
    printf("4. Leave Group:\n");
    printf("   Command: LEAVE_GROUP <group_id>\n");
    printf("\n");
    printf("5. List Members of Group:\n");
    printf("   Command: LIST_MEMBERS <group_id>\n");
    printf("\n");
    printf("6. Kick Member (owner only):\n");
    printf("   Command: KICK_MEMBER <group_id> <user_id>\n");
    printf("   Example: KICK_MEMBER 1 5\n");
    printf("\n");
    printf("7. Invite Member (owner only):\n");
    printf("   Command: INVITE_MEMBER <group_id> <user_id>\n");
    printf("\n");
    printf("8. Approve Member (owner only):\n");
    printf("   Command: APPROVE_MEMBER <group_id> <user_id>\n");
    printf("\n");
    printf("\n--- FILE MANAGEMENT ---\n");
    printf("9. List Files/Folders:\n");
    printf("   Command: LIST [subfolder]\n");
    printf("   Example: LIST          (root folder)\n");
    printf("   Example: LIST myfolder (inside folder)\n");
    printf("\n");
    printf("10. Upload File:\n");
    printf("    Command: UPLOAD <filename>\n");
    printf("    Example: UPLOAD document.pdf\n");
    printf("\n");
    printf("11. Download File:\n");
    printf("    Command: DOWNLOAD <filename>\n");
    printf("    Example: DOWNLOAD report.docx\n");
    printf("\n");
    printf("12. Create Folder:\n");
    printf("    Command: MKDIR <foldername>\n");
    printf("    Example: MKDIR Documents\n");
    printf("\n");
    printf("13. Delete File/Folder:\n");
    printf("    Command: DELETE <itemname>\n");
    printf("    Example: DELETE oldfile.txt\n");
    printf("\n");
    printf("14. Rename File/Folder:\n");
    printf("    Command: RENAME <oldname> <newname>\n");
    printf("    Example: RENAME old.txt new.txt\n");
    printf("\n");
    printf("15. Move File/Folder:\n");
    printf("    Command: MOVE <source> <destination>\n");
    printf("    Example: MOVE file.txt backup/\n");
    printf("\n");
    printf("16. Copy File/Folder:\n");
    printf("    Command: COPY <source> <destination>\n");
    printf("    Example: COPY data.csv backup/\n");
    printf("\n");
    printf("17. Show This Menu:\n");
    printf("    Command: HELP\n");
    printf("\n");
    printf("18. Exit Application:\n");
    printf("    Command: EXIT\n");
    printf("====================================\n");
}