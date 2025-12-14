#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

#include "network.h"

// --- LOW LEVEL WRAPPERS ---

int send_all(int sockfd, const void *buffer, size_t len) {
    size_t total_sent = 0;      // Total bytes sent so far
    size_t bytes_left = len;    // Bytes remaining to be sent
    int n;

    while (total_sent < len) {
        // Send data starting from the current offset
        n = send(sockfd, (const char *)buffer + total_sent, bytes_left, 0);
        
        if (n == -1) { 
            perror("send_all error");
            return -1; 
        }
        
        total_sent += n;
        bytes_left -= n;
    }
    
    return 0; // Success
}

int recv_all(int sockfd, void *buffer, size_t len) {
    size_t total_received = 0;
    size_t bytes_left = len;
    int n;

    while (total_received < len) {
        n = recv(sockfd, (char *)buffer + total_received, bytes_left, 0);
        
        if (n == 0) {
            return -2; // Connection closed by peer
        }
        if (n == -1) {
            perror("recv_all error");
            return -1; // Network error
        }

        total_received += n;
        bytes_left -= n;
    }

    return 0; // Success
}

// --- HIGH LEVEL PROTOCOL HANDLERS ---

int send_packet(int sockfd, int type, const void *payload, int payload_len) {
    PacketHeader header;
    
    // 1. Prepare Header
    header.type = type;
    header.payload_len = payload_len;

    // 2. Send Header first (Fixed size)
    if (send_all(sockfd, &header, sizeof(PacketHeader)) < 0) {
        return -1;
    }

    // 3. Send Payload (if any)
    if (payload_len > 0 && payload != NULL) {
        if (send_all(sockfd, payload, payload_len) < 0) {
            return -1;
        }
    }

    return 0;
}

int recv_packet(int sockfd, int *type, void *payload_buffer) {
    PacketHeader header;
    int status;

    // 1. Receive Header first
    status = recv_all(sockfd, &header, sizeof(PacketHeader));
    if (status != 0) {
        // Return -1 to indicate connection issue or close
        return -1; 
    }

    // Output the message type
    *type = header.type;

    // 2. Process Payload
    if (header.payload_len > 0) {
        // Safety check: Prevent buffer overflow
        if (header.payload_len > BUFFER_SIZE) {
            fprintf(stderr, "Error: Payload size (%d) exceeds BUFFER_SIZE (%d)\n", 
                    header.payload_len, BUFFER_SIZE);
            // Optionally: flush socket here, but returning error is safer
            return -1;
        }

        // Receive the actual body data
        status = recv_all(sockfd, payload_buffer, header.payload_len);
        if (status != 0) {
            return -1;
        }

        // Safety: Null-terminate the string (useful if payload is text)
        // Ensure payload_buffer is allocated with at least (payload_len + 1)
        ((char*)payload_buffer)[header.payload_len] = '\0';
    } else {
        // No payload, empty the string
        ((char*)payload_buffer)[0] = '\0';
    }

    return header.payload_len; // Return number of bytes read
}