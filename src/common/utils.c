#include <stdio.h>
#include <time.h>
#include <string.h>
#include "common.h"

/**
 * @brief Logs a message to the server log file with a timestamp.
 * @param msg The message string to log.
 */
void log_activity(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        perror("Cannot open log file");
        return;
    }

    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    // Format: YYYY-MM-DD HH:MM:SS
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    // Write to file
    fprintf(f, "[%s] %s\n", time_str, msg);
    
    // Also print to server console for monitoring
    printf("[%s] %s\n", time_str, msg);

    fclose(f);
}