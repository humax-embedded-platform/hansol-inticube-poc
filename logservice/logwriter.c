#include "logwriter.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define LOG_FILE_PATH "./"
#define MAX_LOG_FILE_SIZE (500 * 1024 * 1024) // 200 MB

static task_t logwriter_task;
static size_t current_log_size = 0;

static int logwriter_init_log_file(logwriter_t* writer) {
    char filename[256];
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);

    snprintf(filename, sizeof(filename), LOG_FILE_PATH "logfile_%04d%02d%02d_%02d%02d%02d.log",
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    int new_log_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (new_log_fd < 0) {
        perror("Failed to open log file");
        return -1;
    }

    if(writer->log_file_fd != 0) {
        close(writer->log_file_fd);
    }

    writer->log_file_fd = new_log_fd;

    return 0;
}

static void logwriter_worker_func(void* arg) {
    logwriter_t* writer = (logwriter_t*)arg;
    log_entry_t entry;

    while (1) {
        if (buffer_read(&writer->logbuff, &entry) == 0) {
            ssize_t bytes_written = write(writer->log_file_fd, entry.data, entry.size);
            if (bytes_written < 0) {
                perror("Failed to write log message to file");
            }
            
            buffer_log_entry_deinit(&entry);

            current_log_size += bytes_written;
            if(current_log_size > MAX_LOG_FILE_SIZE) {
                logwriter_init_log_file(writer);
                current_log_size = 0;
            }
        } else {
            usleep(1000);
        }
    }
}

void logwriter_init(logwriter_t *writer) {

    writer->log_file_fd = 0;

    logwriter_init_log_file(writer);

    buffer_init(&writer->logbuff, LOG_WRITER_CAPACITY);

    logwriter_task.task_handler = logwriter_worker_func;
    logwriter_task.arg = (void*)writer;
    if (worker_init(&writer->writer, &logwriter_task) < 0) {
        perror("Failed to initialize worker");
        close(writer->log_file_fd);
        buffer_deinit(&writer->logbuff);
        exit(EXIT_FAILURE);
    }
}

void logwriter_deinit(logwriter_t *writer) {
    worker_deinit(&writer->writer);
    buffer_deinit(&writer->logbuff);
    close(writer->log_file_fd);
}

void logwriter_write_log(logwriter_t *writer, char* buff, size_t len) {
    buffer_write(&writer->logbuff, buff, len);
}
