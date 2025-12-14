Here is the consolidated **`README.md`** file. You can copy the code block below and paste it directly into your `README.md` file.

```markdown
# Network Programming Project: File Sharing Application

This is a multi-threaded Client-Server application written in C using TCP Sockets. It features a custom application-layer protocol, session management, and file transfer capabilities.

**Current Phase:** Module 1 (Core Architecture, Authentication, and Session Management).

---

## 📂 1. Project Structure

The project is organized to separate logic, headers, and data.

```text
File-sharing-app/
├── Makefile                 # Automation script for compiling and running
├── .gitignore               # Git configuration
├── README.md                # Project documentation
├── data/                    # Runtime storage
│   ├── users.txt            # User database (ID | Username | Password)
│   ├── server.log           # Server activity logs
│   └── files/               # Folder for uploaded files
├── include/                 # Header files (.h) - The "Contract"
│   ├── common.h             # Global config (Ports, Structs)
│   ├── protocol.h           # Protocol definitions (Packet Header, MessageType)
│   ├── network.h            # Network function prototypes
│   ├── db.h                 # Database function prototypes
│   └── utils.h              # Utility prototypes
└── src/                     # Source code (.c)
    ├── common/              # Shared code between Client & Server
    │   ├── network.c        # TCP stream handling & Packet encapsulation
    │   ├── db.c             # Text-file database operations
    │   └── utils.c          # Logging functions
    ├── server/              # Server-side logic
    │   ├── main.c           # Socket setup, Thread creation, Accept loop
    │   ├── session_mgr.c    # Manages online users (Thread-safe)
    │   ├── request_handler.c# Request routing (Switch-case)
    │   └── handle_auth.c    # Login/Register logic
    └── client/              # Client-side logic
        ├── main.c           # Connection setup
        ├── client_net.c     # Main loop using select() (I/O Multiplexing)
        └── client_ui.c      # User Interface & Menu
```

---

## 🛠 2. Module 1: Core Functionality Explained

### A. Network Layer (`src/common/network.c`)
Handles the complexities of TCP streams.
*   **`send_all` / `recv_all`**: Ensures that the exact number of bytes is sent/received, solving the TCP fragmentation issue.
*   **`send_packet` / `recv_packet`**: Wraps data into our custom protocol. It sends a **Header** (Type + Length) first, followed by the **Payload**.

### B. Database Layer (`src/common/db.c`)
Handles persistent data using simple text files.
*   **`db_check_login`**: Scans `data/users.txt` to verify credentials.
*   **`db_register_user`**: Checks for duplicates and appends new users with an auto-incremented ID.

### C. Session Management (`src/server/session_mgr.c`)
Manages connected clients in the Server's memory.
*   **Thread Safety**: Uses `pthread_mutex` to ensure multiple threads don't corrupt the session list.
*   **`add_session`**: Maps a Socket FD to client info (IP, Login Status).

### D. Client Logic (`src/client/client_net.c`)
The heart of the client application.
*   **`client_main_loop`**: Uses **`select()`** to monitor both the **Keyboard (STDIN)** and the **Server Socket** simultaneously. This allows the client to receive messages instantly without waiting for user input.

---

## 🚀 3. How to Build & Run

We use a **Makefile** to automate everything.

### Step 1: Compilation
Run the following command in the project root to compile both Server and Client:
```bash
make
```

### Step 2: Run Server
The server must be started first. It listens on port **3636**.
```bash
# Using Makefile shortcut
make run_server

# OR Manual execution
./bin/server
```

### Step 3: Run Client
Open a **new terminal** window and run:
```bash
# Using Makefile shortcut (connects to localhost)
make run_client

# OR Manual execution (IP and Port required)
./bin/client 127.0.0.1 3636
```

### Step 4: Clean Up
To remove compiled binaries and object files:
```bash
make clean
```

---

## 📝 4. Usage Guide (Test Cases)

Once the client is running, you will see a menu. You can use the following commands:

**1. Register a new account:**
```text
> REGISTER username password
Example: REGISTER admin 123456
```

**2. Login:**
```text
> LOGIN username password
Example: LOGIN admin 123456
```

**3. Exit:**
```text
> EXIT
```

---

## ⚠️ Notes for Developers
1.  **Do not commit** files inside `bin/` or `data/` (except `.keep` files).
2.  Always use **`make`** to compile to ensure all headers are linked correctly.
3.  If you change `include/protocol.h`, notify the team as it affects both Client and Server.
```