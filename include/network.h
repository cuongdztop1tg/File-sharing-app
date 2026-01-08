#ifndef NETWORK_H
#define NETWORK_H

#include <sys/types.h>
#include "common.h"   // To get BUFFER_SIZE
#include "protocol.h" // To get struct PacketHeader, MessageType

/**
 * @brief Sends exactly 'len' bytes of data.
 * Handles the nature of TCP streams where a single send() call 
 * might not send all requested data.
 * 
 * @param sockfd The socket file descriptor.
 * @param buffer Pointer to the data to send.
 * @param len The number of bytes to send.
 * @return 0 on success, -1 on failure.
 */
int send_all(int sockfd, const void *buffer, size_t len);

/**
 * @brief Receives exactly 'len' bytes of data.
 * Loops recv() until the specific amount of data is received.
 * 
 * @param sockfd The socket file descriptor.
 * @param buffer Pointer to the buffer where data will be stored.
 * @param len The number of bytes to receive.
 * @return 0 on success, -1 on error, -2 if the connection was closed by peer.
 */
int recv_all(int sockfd, void *buffer, size_t len);

/**
 * @brief Encapsulates and sends a complete Message (Header + Payload).
 * 
 * @param sockfd The destination socket.
 * @param type The message type (MSG_LOGIN, MSG_UPLOAD, etc.).
 * @param payload The actual data (struct, string, or binary content).
 * @param payload_len The length of the payload in bytes.
 * @return 0 on success, -1 on failure.
 */
int send_packet(int sockfd, int type, const void *payload, int payload_len);

/**
 * @brief Receives a complete Message.
 * Automatically receives the Header first to determine payload length,
 * then receives the Payload.
 * 
 * @param sockfd The source socket.
 * @param type (Output) Pointer to store the received message type.
 * @param payload_buffer (Output) Buffer to store the received data.
 *                       (Will be null-terminated if it's text).
 * @return The payload length on success, -1 on error/disconnect.
 */
int recv_packet(int sockfd, int *type, void *payload_buffer);

#endif // NETWORK_H