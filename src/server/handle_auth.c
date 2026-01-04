#include <stdio.h>
#include <string.h>
#include "common.h"
#include "protocol.h"
#include "network.h"

// Forward declarations (should be in headers)
int db_check_login(const char *username, const char *password);
int db_register_user(const char *username, const char *password);
int db_change_password(int user_id, const char *new_password);
Session *find_session(int sockfd);
void log_activity(const char *msg);

void handle_login(int sockfd, char *payload) {
    // 1. Lấy thông tin Session hiện tại
    Session *sess = find_session(sockfd);
    
    // Safety check (hiếm khi xảy ra nếu code chạy đúng)
    if (sess == NULL) {
        send_packet(sockfd, MSG_ERROR, "Session Error", 13);
        return;
    }

    // --- LOGIC MỚI: KIỂM TRA ĐÃ LOGIN CHƯA ---
    if (sess->is_logged_in) {
        char msg[256];
        sprintf(msg, "Login failed: You are already logged in as '%s'. Please LOGOUT first.", sess->username);
        send_packet(sockfd, MSG_ERROR, msg, strlen(msg));
        
        // Ghi log cảnh báo
        char log_msg[300];
        sprintf(log_msg, "User '%s' (ID %d) attempted to re-login without logout.", sess->username, sess->user_id);
        log_activity(log_msg);
        
        return; // <--- Dừng hàm tại đây, không xử lý tiếp
    }
    // -----------------------------------------

    char user[50], pass[50];
    
    // Parse payload: "username password"
    if (sscanf(payload, "%s %s", user, pass) < 2) {
        char *msg = "Missing username or password";
        send_packet(sockfd, MSG_ERROR, msg, strlen(msg));
        return;
    }

    // ... (Phần code kiểm tra DB cũ giữ nguyên ở dưới) ...
    int user_id = db_check_login(user, pass);
    
    if (user_id != -1) {
        // Update Session
        sess->user_id = user_id;
        sess->is_logged_in = 1;
        strcpy(sess->username, user);

        // Send Success Response
        char msg[100];
        sprintf(msg, "Login successful. Welcome %s (ID: %d)", user, user_id);
        send_packet(sockfd, MSG_SUCCESS, msg, strlen(msg));
        
        // Log
        char log_msg[200];
        sprintf(log_msg, "User '%s' (ID %d) logged in from %s", 
                user, user_id, sess->client_ip);
        log_activity(log_msg);

    } else {
        char *msg = "Invalid username or password";
        send_packet(sockfd, MSG_ERROR, msg, strlen(msg));
        log_activity("Failed login attempt.");
    }
}

void handle_register(int sockfd, char *payload) {
    Session *sess = find_session(sockfd);
    if (sess && sess->is_logged_in) {
        char msg[256];
        sprintf(msg, "Registration failed: You are currently logged in as '%s'. Please LOGOUT first.", sess->username);
        send_packet(sockfd, MSG_ERROR, msg, strlen(msg));
        return; 
    }
    
    char user[50], pass[50];
    
    
    if (sscanf(payload, "%s %s", user, pass) < 2) {
        send_packet(sockfd, MSG_ERROR, "Invalid format", 14);
        return;
    }

    int new_id = db_register_user(user, pass);
    
    if (new_id != -1) {
        char msg[100];
        sprintf(msg, "Registration successful. Please login. ID: %d", new_id);
        send_packet(sockfd, MSG_SUCCESS, msg, strlen(msg));
        
        char log_msg[200];
        sprintf(log_msg, "New user registered: %s (ID %d)", user, new_id);
        log_activity(log_msg);
    } else {
        send_packet(sockfd, MSG_ERROR, "Username already exists", 23);
    }
}

/**
 * @brief Xử lý yêu cầu đăng xuất từ client
 */
void handle_logout(int sockfd, char *payload) {
    // 1. Tìm session của socket hiện tại
    Session *sess = find_session(sockfd);
    
    // Safety check
    if (sess == NULL) {
        return; 
    }

    // 2. Kiểm tra xem user đã login chưa
    if (sess->is_logged_in == 0) {
        char *msg = "Logout failed: You are not logged in.";
        send_packet(sockfd, MSG_ERROR, msg, strlen(msg));
        return;
    }

    // 3. Thực hiện Logout: Reset thông tin Session về trạng thái Guest
    int old_uid = sess->user_id;
    char old_user[50];
    strcpy(old_user, sess->username);

    sess->user_id = -1;
    sess->is_logged_in = 0;
    strcpy(sess->username, "Guest");

    // 4. Gửi phản hồi thành công
    char msg[100];
    sprintf(msg, "Goodbye %s! You have been logged out.", old_user);
    send_packet(sockfd, MSG_SUCCESS, msg, strlen(msg));

    // 5. Ghi log
    char log_msg[200];
    sprintf(log_msg, "User '%s' (ID %d) logged out.", old_user, old_uid);
    log_activity(log_msg);
}

void handle_change_password(int sockfd, char *payload) {
    Session *sess = find_session(sockfd);
    
    // 1. Kiểm tra đã login chưa
    if (!sess || !sess->is_logged_in) {
        send_packet(sockfd, MSG_ERROR, "Error: You must login first.", 50);
        return;
    }

    char old_pass[50], new_pass[50];
    
    // 2. Parse payload: "old_pass new_pass"
    if (sscanf(payload, "%s %s", old_pass, new_pass) < 2) {
        send_packet(sockfd, MSG_ERROR, "Usage: CHANGE_PASS <old> <new>", 50);
        return;
    }

    // 3. Xác thực mật khẩu cũ (Tận dụng hàm db_check_login có sẵn)
    // db_check_login trả về ID nếu đúng user/pass
    int check_id = db_check_login(sess->username, old_pass);
    
    if (check_id == sess->user_id) {
        // 4. Nếu mật khẩu cũ đúng, tiến hành cập nhật
        if (db_change_password(sess->user_id, new_pass) == 0) {
            char msg[100];
            sprintf(msg, "Password changed successfully for user '%s'.", sess->username);
            send_packet(sockfd, MSG_SUCCESS, msg, strlen(msg));
            
            log_activity("User changed password successfully.");
        } else {
            send_packet(sockfd, MSG_ERROR, "Database error updating password.", 31);
        }
    } else {
        send_packet(sockfd, MSG_ERROR, "Current password is incorrect.", 29);
        log_activity("Failed password change attempt (wrong old pass).");
    }
}