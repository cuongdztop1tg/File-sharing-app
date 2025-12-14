# 1. Trình biên dịch và Cờ (Flags)
CC = gcc
# -Iinclude: Giúp trình biên dịch tìm file .h trong folder include/
# -pthread: Hỗ trợ đa luồng (cho Server)
# -Wall: Hiện tất cả cảnh báo (Warning) để dễ debug
CFLAGS = -Wall -pthread -Iinclude

# 2. Thư mục đầu ra
BIN_DIR = bin

# 3. Danh sách các file nguồn (Source files)
# Phần dùng chung (Common)
COMMON_SRC = src/common/network.c \
             src/common/db.c \
             src/common/utils.c

# Phần Server (Bao gồm cả Common)
SERVER_SRC = src/server/main.c \
             src/server/request_handler.c \
             src/server/session_mgr.c \
             src/server/handle_auth.c \
             src/server/handle_group.c \
             src/server/handle_file.c \
             $(COMMON_SRC)

# Phần Client (Bao gồm cả Common)
CLIENT_SRC = src/client/main.c \
             src/client/client_net.c \
             src/client/client_ui.c \
             src/client/file_transfer.c \
             $(COMMON_SRC)

# 4. Các mục tiêu (Targets)
# Gõ 'make' sẽ chạy mục tiêu 'all'
all: create_dirs server client

# Compile Server
server: $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/server $(SERVER_SRC)

# Compile Client
client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/client $(CLIENT_SRC)

# Tạo thư mục bin nếu chưa có
create_dirs:
	mkdir -p $(BIN_DIR)
	mkdir -p data/files

# Dọn dẹp file biên dịch cũ (Gõ 'make clean')
clean:
	rm -rf $(BIN_DIR)/*

# Chạy thử Server nhanh (Gõ 'make run_server')
run_server: server
	./$(BIN_DIR)/server

# Chạy thử Client nhanh (Gõ 'make run_client')
run_client: client
	./$(BIN_DIR)/client