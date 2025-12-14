#include <stdio.h>
#include "common.h"

/**
 * @brief Prints the main menu commands.
 */
void print_main_menu() {
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
void print_help() {
    print_main_menu();
}