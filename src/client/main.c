#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "common.h"
#include "network.h"

// --- EXTERNAL FUNCTION PROTOTYPES ---
// Khai báo prototype hàm từ client_net.c và client_ui.c
// (Nếu bạn đã tạo file header .h thì include vào thay vì khai báo ở đây)
void client_main_loop(int sockfd); 
void print_main_menu();

int main(int argc, char *argv[]) {
    // 1. Check arguments
    if (argc < 3) {
        printf("Usage: %s <Server_IP> <Port>\n", argv[0]);
        return 0;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    printf("Connecting to Server %s:%d ...\n", server_ip, port);

    // 2. Create Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 3. Connect to Server
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to Server Successfully!\n");

    // 4. ENTER MAIN LOOP (Logic nằm ở client_net.c)
    // Hàm này sẽ dùng select() để đợi nhập liệu từ bàn phím hoặc tin nhắn từ server
    client_main_loop(sockfd);

    // 5. Cleanup (Chỉ chạy khi user gõ EXIT)
    close(sockfd);
    printf("Client disconnected.\n");
    
    return 0;
}