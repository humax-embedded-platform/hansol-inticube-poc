#include "logrecv.h"
#include "logwriter.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>

task_t logrecv_task;

static void logrecv_worker_func(void* arg) {
    logrecv_t* receiver = (logrecv_t*)arg;
    char buffer[1024*50];

    while (1) {
        ssize_t bytes_received = recv(receiver->logrecv_fd, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            logwriter_write_log(&receiver->writer, buffer, bytes_received);
        } else if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Failed to receive log message");
        }
    }
}

void logrecv_init(logrecv_t* receiver) {
    logwriter_init(&receiver->writer);

    receiver->logrecv_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (receiver->logrecv_fd < 0) {
        perror("Failed to create socket");
        logwriter_deinit(&receiver->writer);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, LOGRECV_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(LOGRECV_SOCKET_PATH);

    if (bind(receiver->logrecv_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind socket");
        close(receiver->logrecv_fd);
        logwriter_deinit(&receiver->writer);
        exit(EXIT_FAILURE);
    }

    logrecv_task.task_handler = logrecv_worker_func;
    logrecv_task.arg = (void*)receiver;

    if (worker_init(&receiver->recv_worker, &logrecv_task) < 0) {
        perror("Failed to initialize worker");
        close(receiver->logrecv_fd);
        logwriter_deinit(&receiver->writer);
        exit(EXIT_FAILURE);
    }
}

void logrecv_deinit(logrecv_t* receiver) {
    worker_deinit(&receiver->recv_worker);
    logwriter_deinit(&receiver->writer);
    close(receiver->logrecv_fd);
    unlink(LOGRECV_SOCKET_PATH);
}
