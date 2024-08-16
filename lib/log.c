#include "log.h"
#include "worker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>

#define LOG_SERVICE_SOCKET_PATH "/tmp/log_service_socket"
#define LOG_WRITE_RETRY_MAX     5

static log_client_t log_client;
static task_t       logtask;

static char write_buff[LOG_BUFFER_FLUSH_THRESHOLD_SIZE];
static int  write_buff_count = 0;

void log_worker_func(void* arg) {
    log_client_t* client = (log_client_t*)arg;
    log_entry_t log;
    int seq_number = 0;
    char formatted_entry[LOG_FRAME_MAX_SIZE];
    int write_start_index = 0;
    int retry_count = 0;

    while (1) {
        if (buffer_read(&client->buffer, &log) == 0) {
            int formatted_size = snprintf(formatted_entry, sizeof(formatted_entry),
                                          "seq: %d  -  %.*s\n", seq_number++, (int)log.size, log.data);
            buffer_log_entry_deinit(&log);

            if (write_buff_count + formatted_size <= sizeof(write_buff)) {
                memcpy(write_buff + write_buff_count, formatted_entry, formatted_size);
                write_buff_count += formatted_size;
            } else {
                write_start_index = 0;
                retry_count = 0;
                while (write_buff_count > 0)
                {
                    ssize_t bytes_written = send(client->log_fd, write_buff + write_start_index , write_buff_count, 0);
                    if (bytes_written < 0) {
                        retry_count ++;
                        if(retry_count >= LOG_WRITE_RETRY_MAX) {
                            write_buff_count = 0;
                            break;
                        }
                        perror("Failed to send log data");
                    } else {
                        write_start_index += bytes_written;
                        write_buff_count -= bytes_written;
                        retry_count = 0;
                    }
                }

                // Reset buffer
                write_buff_count = 0;
                memcpy(write_buff + write_buff_count, formatted_entry, formatted_size);
                write_buff_count += formatted_size;
            }
        } else {
            if (write_buff_count > 0) {
                write_start_index = 0;
                retry_count = 0;
                while (write_buff_count > 0)
                {
                    ssize_t bytes_written = send(client->log_fd, write_buff + write_start_index , write_buff_count, 0);
                    if (bytes_written < 0) {
                        retry_count ++;
                        if(retry_count >= LOG_WRITE_RETRY_MAX) {
                            write_buff_count = 0;
                            break;
                        }
                        perror("Failed to send log data");
                    } else {
                        write_start_index += bytes_written;
                        write_buff_count -= bytes_written;
                        retry_count = 0;
                    }
                }
            }

            if(atomic_load(&log_client.is_initialized) == 0) {
                break;
            }

            usleep(1000);
        }
    }
}

void log_init() {

    if(atomic_load(&log_client.is_initialized) == 1) {
        printf("log client is initialized, not need init again\n");
        return;
    }

    buffer_init(&log_client.buffer, LOG_BUFFER_CAPACITY);

    log_client.log_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (log_client.log_fd < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(log_client.log_fd, F_GETFL, 0);
    if (flags < 0) {
        perror("Failed to get socket flags");
        close(log_client.log_fd);
        exit(EXIT_FAILURE);
    }

    if (fcntl(log_client.log_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("Failed to set socket to non-blocking mode");
        close(log_client.log_fd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, LOG_SERVICE_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(log_client.log_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to connect to log service");
        close(log_client.log_fd);
        exit(EXIT_FAILURE);
    }

    logtask.task_handler = log_worker_func;
    logtask.arg = (void*)&log_client;
    if (worker_init(&log_client.worker, &logtask) < 0) {
        perror("Failed to initialize worker");
        close(log_client.log_fd);
        exit(EXIT_FAILURE);
    }

    atomic_store(&log_client.is_initialized, 1);
}

void log_deinit() {
    atomic_store(&log_client.is_initialized, 0);

    worker_deinit(&log_client.worker);

    log_entry_t entry;
    while ((buffer_read(&log_client.buffer, &entry)) == 0) {
        send(log_client.log_fd, entry.data, entry.size, 0);
        buffer_log_entry_deinit(&entry);
    }

    buffer_deinit(&log_client.buffer);

    close(log_client.log_fd);
}

void log_write(char* buff, size_t len) {
    if (atomic_load(&log_client.is_initialized) == 0) {
        fprintf(stderr, "Logging is disabled. Cannot write to buffer.\n");
        return;
    }

    buffer_write(&log_client.buffer, buff, len);
}
