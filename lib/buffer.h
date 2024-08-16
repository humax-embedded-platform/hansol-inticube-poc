#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <pthread.h>

typedef struct {
    char *data;    // Pointer to dynamically allocated log data
    size_t size;   // Size of the log data
    int retry;     // Retry count for the log entry
} log_entry_t;

typedef struct {
    log_entry_t *entries;  // Pointer to dynamically allocated array of log entries
    size_t head;           // Index of the head of the buffer
    size_t tail;           // Index of the tail of the buffer
    size_t count;          // Current count of entries in the buffer
    size_t capacity;       // Capacity of the buffer (number of log entries it can hold)
    pthread_mutex_t mutex; // Mutex for thread-safe access to the buffer
} buffer_t;

// Function declarations
void buffer_init(buffer_t* cb, size_t capacity);
void buffer_write(buffer_t* cb, const char* data, size_t len);
void buffer_write_with_retry(buffer_t* cb, const char* data, size_t len, int retry_count);
int  buffer_read(buffer_t* cb, log_entry_t* log);
void buffer_deinit(buffer_t* cb);

void buffer_log_entry_deinit(log_entry_t* log);
#endif // BUFFER_H
