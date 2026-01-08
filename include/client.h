#ifndef CLIENT_H
#define CLIENT_H

// --- Client network functions (client_net.c) ---

/**
 * @brief Main client loop using select() for I/O multiplexing
 * @param sockfd Connected socket file descriptor
 */
void client_main_loop(int sockfd);

/**
 * @brief Handles incoming messages from server
 * @param sockfd Socket file descriptor
 */
void handle_server_message(int sockfd);

/**
 * @brief Processes user keyboard input and sends commands
 * @param sockfd Socket file descriptor
 */
void handle_user_input(int sockfd);

/**
 * @brief Prints the initial command menu
 */
void print_menu();

// --- Client UI functions (client_ui.c) ---

/**
 * @brief Prints main menu (before login)
 */
void print_main_menu();

/**
 * @brief Prints logged-in user menu with all available commands
 */
void print_logged_in_menu();

/**
 * @brief Displays help information
 */
void print_help();

// --- File transder functions (file_transfer.c) ---

/**
 * @brief Uploads a file to the server
 * @param sockfd Socket file descriptor
 * @param filename Name/path of file to upload
 */
void upload_file(int sockfd, char *filename);

/**
 * @brief Downloads a file from the server
 * @param sockfd Socket file descriptor
 * @param filename Name of file to download
 */
void download_file(int sockfd, char *filename);

/**
 * @brief Gets the size of a file (utility function)
 * @param filename Path to the file
 * @return File size in bytes, or -1 on error
 */
long get_file_size(const char *filename);

#endif // CLIENT_H