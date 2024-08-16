#ifndef LOGWRITER_H
#define LOGWRITER_H

#include "worker.h"
#include "buffer.h"
#include <stdatomic.h>

#define LOG_WRITER_BUFFER_SIZE LOG_BUFFER_FLUSH_THRESHOLD_SIZE
#define LOG_WRITER_CAPACITY    100

typedef struct logwriter_t {
    int log_file_fd;
    worker_t writer;
    buffer_t logbuff;
} logwriter_t;

void logwriter_init(logwriter_t *writer);
void logwriter_deinit(logwriter_t *writer);
void logwriter_write_log(logwriter_t *writer, char* buff, size_t len);

#endif // LOGWRITER_H
