#include "log.h"
#include "worker.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define LOG_WRITE_RETRY_MAX     5
#define LOGSERVER_EXECUTABLE_FILE "./logservice"

#define LOG_INIT_STATUS_INITIALIZED     1
#define LOG_INIT_STATUS_UNINITIALIZED   0

static log_client_t log_client;
static task_t       logtask;

static char write_buff[LOG_BUFFER_FLUSH_THRESHOLD_SIZE];
static int  write_buff_count = 0;

static int start_log_server(void) {
    if (log_client.logserver_pid > 0) {
        printf("Log server is already running with PID: %d\n", log_client.logserver_pid);
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Failed to fork process for log server");
        return -1;
    }

    if (pid == 0) {
        if (execlp(LOGSERVER_EXECUTABLE_FILE, LOGSERVER_EXECUTABLE_FILE, (char *)NULL) == -1) {
            perror("Failed to start log server");
            return -1;
        }
    } else {
        log_client.logserver_pid = pid;
    }

    return 0;
}

static void stop_log_server(void) {
    if (log_client.logserver_pid <= 0) {
        printf("Log server is not running.\n");
        return;
    }

    if (kill(log_client.logserver_pid, SIGTERM) == -1) {
        perror("Failed to send SIGTERM to log server");
        return;
    }


    int status;
    pid_t result = waitpid(log_client.logserver_pid, &status, 0);

    log_client.logserver_pid = -1;
}

static int log_set_init_status(int status) {
    pthread_mutex_lock(&log_client.m);
    log_client.is_initialized = status;
    pthread_mutex_unlock(&log_client.m);
}

static int log_get_init_status() {
    int status = 0;

    pthread_mutex_lock(&log_client.m);
    status = log_client.is_initialized;
    pthread_mutex_unlock(&log_client.m);

    return status;
}

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

            if(log_get_init_status() == LOG_INIT_STATUS_UNINITIALIZED) {
                break;
            }

            usleep(1000);
        }
    }
}

int log_init(const char* log_path) {

    if(start_log_server() < 0) {
        return -1;
    }

    buffer_init(&log_client.buffer, LOG_BUFFER_CAPACITY);

    log_client.log_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (log_client.log_fd < 0) {
        perror("Failed to create socket");
        return -1;
    }

    int flags = fcntl(log_client.log_fd, F_GETFL, 0);
    if (flags < 0) {
        perror("Failed to get socket flags");
        close(log_client.log_fd);
        return -1;
    }

    if (fcntl(log_client.log_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("Failed to set socket to non-blocking mode");
        close(log_client.log_fd);
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, LOGRECV_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    while (connect(log_client.log_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        usleep(100*1000);
    }

    logtask.task_handler = log_worker_func;
    logtask.arg = (void*)&log_client;
    if (worker_init(&log_client.worker, &logtask) < 0) {
        perror("Failed to initialize worker");
        close(log_client.log_fd);
        return -1;
    }

    if (pthread_mutex_init(&log_client.m, NULL) != 0) {
        perror("Mutex initialization failed");
        close(log_client.log_fd);
        return -1;
    }

    log_set_init_status(LOG_INIT_STATUS_INITIALIZED);

    if(log_path != NULL) {
        log_config(CONFIG_LOG_PATH, log_path);
    }

    return 0;
}

void log_deinit() {
    log_set_init_status(LOG_INIT_STATUS_UNINITIALIZED);
    worker_deinit(&log_client.worker);

    log_entry_t entry;
    while ((buffer_read(&log_client.buffer, &entry)) == 0) {
        send(log_client.log_fd, entry.data, entry.size, 0);
        buffer_log_entry_deinit(&entry);
    }

    pthread_mutex_destroy(&log_client.m);
    buffer_deinit(&log_client.buffer);
    close(log_client.log_fd);
    stop_log_server();
}

void log_write(char* buff, size_t len) {
    if (log_get_init_status() ==  LOG_INIT_STATUS_UNINITIALIZED) {
        printf("Logging is disabled. Cannot write to buffer.\n");
        return;
    }

    buffer_write(&log_client.buffer, buff, len);
}

void log_config(int id,const char* data) {
    int sockfd;
    struct sockaddr_un addr;
    config_msg_t msg;

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CONFIG_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    msg.config_id = id;
    strncpy(msg.data, data, sizeof(msg.data) - 1);
    msg.data[sizeof(msg.data) - 1] = '\0';

    if (sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) < 0) {
        perror("sendto failed");
    }

    close(sockfd);
}