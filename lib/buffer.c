#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void buffer_init(buffer_t* cb, size_t capacity) {
    cb->capacity = capacity;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;

    cb->entries = (log_entry_t*)malloc(capacity * sizeof(log_entry_t));
    if (cb->entries == NULL) {
        perror("Failed to allocate memory for buffer entries");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < capacity; i++) {
        cb->entries[i].data = NULL;
        cb->entries[i].size = 0;
        cb->entries[i].retry = 0;
    }

    if (pthread_mutex_init(&cb->mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        free(cb->entries);
        exit(EXIT_FAILURE);
    }
}

void buffer_write_with_retry(buffer_t* cb, const char* data, size_t len, int retry_count) {
    pthread_mutex_lock(&cb->mutex);

    if (data == NULL || len == 0) {
        pthread_mutex_unlock(&cb->mutex);
        return;
    }

    if (cb->count == cb->capacity) {
        printf("buffer_write_with_retry full\n");
        free(cb->entries[cb->head].data);
        cb->entries[cb->head].data = NULL;
        cb->entries[cb->head].size = 0;
        cb->entries[cb->head].retry = 0;

        cb->head = (cb->head + 1) % cb->capacity;
        cb->count--;
    }

    cb->entries[cb->tail].data = (char*)malloc(len);
    if (cb->entries[cb->tail].data == NULL) {
        perror("Failed to allocate memory for log entry data");
        pthread_mutex_unlock(&cb->mutex);
        return;
    }

    memcpy(cb->entries[cb->tail].data, data, len);
    cb->entries[cb->tail].size = len;
    cb->entries[cb->tail].retry = retry_count;

    cb->tail = (cb->tail + 1) % cb->capacity;
    cb->count++;

    pthread_mutex_unlock(&cb->mutex);
}

void buffer_write(buffer_t* cb, const char* data, size_t len) {
    buffer_write_with_retry(cb, data, len, 0);
}

int buffer_read(buffer_t* cb, log_entry_t* log) {
    pthread_mutex_lock(&cb->mutex);

    if (cb->count == 0) {
        pthread_mutex_unlock(&cb->mutex);
        return -1;
    }

    log_entry_t* entry = &cb->entries[cb->head];

    log->data = (char*)malloc(entry->size);
    if (log->data == NULL) {
        perror("Failed to allocate memory for log entry data");
        pthread_mutex_unlock(&cb->mutex);
        return -1;
    }

    memcpy(log->data, entry->data, entry->size);
    log->size = entry->size;
    log->retry = entry->retry;

    free(entry->data);
    entry->data = NULL;
    entry->size = 0;
    entry->retry = 0;

    cb->head = (cb->head + 1) % cb->capacity;
    cb->count--;

    pthread_mutex_unlock(&cb->mutex);

    return 0;
}

void buffer_deinit(buffer_t* cb) {
    pthread_mutex_lock(&cb->mutex);

    for (size_t i = 0; i < cb->capacity; i++) {
        if (cb->entries[i].data != NULL) {
            free(cb->entries[i].data);
            cb->entries[i].data = NULL;
        }
    }

    free(cb->entries);
    cb->entries = NULL;

    pthread_mutex_unlock(&cb->mutex);
    pthread_mutex_destroy(&cb->mutex);
}

void buffer_log_entry_deinit(log_entry_t* log) {
    if (log->data) {
        free(log->data);
        log->data = NULL;
    }

    log->size = 0;
    log->retry = 0;
}
