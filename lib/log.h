#ifndef LOG_H
#define LOG_H

#include "buffer.h"
#include "worker.h"
#include <stdatomic.h>

#define LOG_BUFFER_CAPACITY 1000
#define LOG_MESAGE_MAX_SIZE 2*1024
#define LOG_FRAME_MAX_SIZE  (LOG_MESAGE_MAX_SIZE + 20)
#define LOG_BUFFER_FLUSH_THRESHOLD_SIZE (LOG_FRAME_MAX_SIZE * 20)


typedef struct log_client_t
{
    int log_fd;
    int logserver_pid;
    buffer_t buffer;
    worker_t worker;
    atomic_int is_initialized;
} log_client_t;

int  log_init(const char* log_path);
void log_deinit();
void log_write(char* buff, size_t len);
void log_config(int id, const char* data);
#endif