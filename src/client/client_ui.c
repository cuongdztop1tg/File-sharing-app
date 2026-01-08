#include <stdio.h>
#include "common.h"

/* ANSI color codes */
#define CLR_RESET   "\033[0m"
#define CLR_TITLE   "\033[1;36m"  /* Cyan đậm */
#define CLR_SECTION "\033[1;33m"  /* Vàng đậm */
#define CLR_CMD     "\033[1;32m"  /* Xanh lá */
#define CLR_EX      "\033[0;37m"  /* Xám nhạt */
#define CLR_WARN    "\033[1;31m"  /* Đỏ đậm */

/**
 * @brief Prints the main menu commands (before login).
 */
void print_main_menu()
{
    /* Clear screen + move cursor home (nếu không thích thì bỏ dòng này) */
    printf("\033[2J\033[H");

    printf(CLR_TITLE);
    printf("+======================================================================+\n");
    printf("|                       FILE SHARING APPLICATION                       |\n");
    printf("+======================================================================+\n");
    printf(CLR_RESET);

    printf("\n");
    printf(CLR_SECTION "● MAIN MENU (Guest)\n" CLR_RESET);
    printf("  Use the commands below to get started:\n\n");

    printf(CLR_CMD  "  [1] REGISTER ACCOUNT\n" CLR_RESET);
    printf("      Command: " CLR_CMD "REGISTER <username> <password>\n" CLR_RESET);
    printf("      Example: " CLR_EX  "REGISTER user1 123456\n\n" CLR_RESET);

    printf(CLR_CMD  "  [2] LOGIN\n" CLR_RESET);
    printf("      Command: " CLR_CMD "LOGIN <username> <password>\n" CLR_RESET);
    printf("      Example: " CLR_EX  "LOGIN user1 123456\n\n" CLR_RESET);

    printf(CLR_CMD  "  [3] EXIT APPLICATION\n" CLR_RESET);
    printf("      Command: " CLR_CMD "EXIT\n" CLR_RESET);
    printf("\n");

    printf(CLR_SECTION "Tip: " CLR_EX "Type the exact command, not the number.\n" CLR_RESET);
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
    printf("\033[2J\033[H"); /* Clear screen */

    printf(CLR_TITLE);
    printf("+======================================================================+\n");
    printf("|                       FILE SHARING APPLICATION                       |\n");
    printf("+======================================================================+\n");
    printf(CLR_RESET);

    printf("\n");
    printf(CLR_SECTION "● USER MAIN MENU\n" CLR_RESET);
    printf("  Below are all supported commands after login.\n\n");

    /* GROUP MANAGEMENT */
    printf(CLR_SECTION "--- GROUP MANAGEMENT -----------------------------------------------\n" CLR_RESET);

    printf(CLR_CMD  "  [1] CREATE GROUP\n" CLR_RESET);
    printf("      Command: " CLR_CMD "CREATE_GROUP <group_name>\n" CLR_RESET);
    printf("      Example: " CLR_EX  "CREATE_GROUP MyProject\n\n" CLR_RESET);

    printf(CLR_CMD  "  [2] LIST ALL GROUPS\n" CLR_RESET);
    printf("      Command: " CLR_CMD "LIST_GROUPS\n\n" CLR_RESET);

    printf(CLR_CMD  "  [3] JOIN GROUP\n" CLR_RESET);
    printf("      Command: " CLR_CMD "JOIN_GROUP <group_id>\n" CLR_RESET);
    printf("      Example: " CLR_EX  "JOIN_GROUP 1\n\n" CLR_RESET);

    printf(CLR_CMD  "  [4] LEAVE GROUP\n" CLR_RESET);
    printf("      Command: " CLR_CMD "LEAVE_GROUP <group_id>\n\n" CLR_RESET);

    printf(CLR_CMD  "  [5] LIST MEMBERS OF GROUP\n" CLR_RESET);
    printf("      Command: " CLR_CMD "LIST_MEMBERS <group_id>\n\n" CLR_RESET);

    printf(CLR_CMD  "  [6] KICK MEMBER" CLR_WARN " (owner only)\n" CLR_RESET);
    printf("      Command: " CLR_CMD "KICK_MEMBER <group_id> <user_id>\n" CLR_RESET);
    printf("      Example: " CLR_EX  "KICK_MEMBER 1 5\n\n" CLR_RESET);

    printf(CLR_CMD  "  [7] INVITE MEMBER" CLR_WARN " (owner only)\n" CLR_RESET);
    printf("      Command: " CLR_CMD "INVITE_MEMBER <group_id> <user_id>\n\n" CLR_RESET);

    printf(CLR_CMD  "  [8] APPROVE MEMBER" CLR_WARN " (owner only)\n" CLR_RESET);
    printf("      Command: " CLR_CMD "APPROVE_MEMBER <group_id> <user_id>\n\n" CLR_RESET);

    printf(CLR_CMD  "  [9] DELETE GROUP" CLR_WARN " (owner only)\n" CLR_RESET);
    printf("      Command: " CLR_CMD "DELETE_GROUP <group_id>\n" CLR_RESET);
    printf("      Example: " CLR_EX  "DELETE_GROUP 1\n\n" CLR_RESET);

    /* FILE MANAGEMENT */
    printf(CLR_SECTION "--- FILE MANAGEMENT -----------------------------------------------\n" CLR_RESET);

    printf(CLR_CMD  "  [10] LIST FILES/FOLDERS\n" CLR_RESET);
    printf("       Command: " CLR_CMD "LIST [subfolder]\n" CLR_RESET);
    printf("       Example: " CLR_EX  "LIST\n" CLR_RESET);
    printf("                " CLR_EX  "LIST myfolder\n\n" CLR_RESET);

    printf(CLR_CMD  "  [11] UPLOAD FILE\n" CLR_RESET);
    printf("       Command: " CLR_CMD "UPLOAD <filename>\n" CLR_RESET);
    printf("       Example: " CLR_EX  "UPLOAD document.pdf\n\n" CLR_RESET);

    printf(CLR_CMD  "  [12] DOWNLOAD FILE\n" CLR_RESET);
    printf("       Command: " CLR_CMD "DOWNLOAD <filename>\n" CLR_RESET);
    printf("       Example: " CLR_EX  "DOWNLOAD report.docx\n\n" CLR_RESET);

    printf(CLR_CMD  "  [13] CREATE FOLDER\n" CLR_RESET);
    printf("       Command: " CLR_CMD "MKDIR <foldername>\n" CLR_RESET);
    printf("       Example: " CLR_EX  "MKDIR Documents\n\n" CLR_RESET);

    printf(CLR_CMD  "  [14] DELETE FILE/FOLDER\n" CLR_RESET);
    printf("       Command: " CLR_CMD "DELETE <itemname>\n" CLR_RESET);
    printf("       Example: " CLR_EX  "DELETE oldfile.txt\n\n" CLR_RESET);

    printf(CLR_CMD  "  [15] RENAME FILE/FOLDER\n" CLR_RESET);
    printf("       Command: " CLR_CMD "RENAME <oldname> <newname>\n" CLR_RESET);
    printf("       Example: " CLR_EX  "RENAME old.txt new.txt\n\n" CLR_RESET);

    printf(CLR_CMD  "  [16] MOVE FILE/FOLDER\n" CLR_RESET);
    printf("       Command: " CLR_CMD "MOVE <source> <destination>\n" CLR_RESET);
    printf("       Example: " CLR_EX  "MOVE file.txt backup/\n\n" CLR_RESET);

    printf(CLR_CMD  "  [17] COPY FILE/FOLDER\n" CLR_RESET);
    printf("       Command: " CLR_CMD "COPY <source> <destination>\n" CLR_RESET);
    printf("       Example: " CLR_EX  "COPY data.csv backup/\n\n" CLR_RESET);

    /* OTHER */
    printf(CLR_SECTION "--- OTHER --------------------------------------------------------\n" CLR_RESET);

    printf(CLR_CMD  "  [18] SHOW THIS MENU\n" CLR_RESET);
    printf("       Command: " CLR_CMD "HELP\n\n" CLR_RESET);

    printf(CLR_CMD  "  [19] EXIT APPLICATION\n" CLR_RESET);
    printf("       Command: " CLR_CMD "EXIT\n\n" CLR_RESET);

    printf(CLR_SECTION "Tip: " CLR_EX "Type the command name + parameters, not the number.\n" CLR_RESET);
}