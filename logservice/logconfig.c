
#include "logconfig.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <limits.h>

#define DEFAULT_LOG_FILE_PATH  "./"
#define LOGSERVER_EXECUTABLE_PATH_MAX 1024

static logconfig_t      config;
static logconfig_item_t config_item;
static task_t           config_task;
static logconfig_changed_cb_t registered_callback = NULL;

static void config_log_path(config_msg_t* config);

static logconfig_handler_t handlers[CONFIG_MAX] = {
    [CONFIG_LOG_PATH] = config_log_path
};

static void config_init_default() {
    char exe_path[LOGSERVER_EXECUTABLE_PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

    if (len == -1) {
        strncpy(config_item.log_path, DEFAULT_LOG_FILE_PATH, sizeof(config_item.log_path) - 1);
        config_item.log_path[sizeof(config_item.log_path) - 1] = '\0';
        return;
    }

    exe_path[len] = '\0';

    char* last_slash = strrchr(exe_path, '/');
    if (last_slash != NULL) {
        *(last_slash + 1) = '\0';
    } else {
        strncpy(config_item.log_path, DEFAULT_LOG_FILE_PATH, sizeof(config_item.log_path) - 1);
        config_item.log_path[sizeof(config_item.log_path) - 1] = '\0';
        return;
    }

    strncpy(config_item.log_path, exe_path, sizeof(config_item.log_path) - 1);
    config_item.log_path[sizeof(config_item.log_path) - 1] = '\0';
}

static void config_log_path(config_msg_t* msg) {
    strncpy(config_item.log_path, msg->data, sizeof(config_item.log_path) - 1);
    config_item.log_path[sizeof(config_item.log_path) - 1] = '\0';
    if(registered_callback) {
        registered_callback(CONFIG_LOG_PATH, &config_item);
    }
}

static int logconfig_is_completed() {
    int is_completed = 0;
    pthread_mutex_lock(&config.m);
    is_completed = config.is_completed;
    pthread_mutex_unlock(&config.m);

    return is_completed;
}

static void logconfig_receiver(void* arg) {
    logconfig_t* config = (logconfig_t*)arg;
    config_msg_t msg;
    ssize_t recv_len;
    int is_completed = false;

    while (1) {
        if(logconfig_is_completed() == 1) {
            break;
        }

        recv_len = recvfrom(config->config_sock_fd, &msg, sizeof(msg), 0, NULL, NULL);
        if (recv_len < 0) {
            continue;
        }

        if (msg.config_id >= 0 && msg.config_id < CONFIG_MAX && handlers[msg.config_id] != NULL) {
            handlers[msg.config_id](&msg);
        } else {
        }
    }
}


int logconfig_init(void) {
    struct sockaddr_un server_addr;

    config_init_default();

    config.config_sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0); 
    if (config.config_sock_fd < 0) {
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, CONFIG_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(CONFIG_SOCKET_PATH);

    if (bind(config.config_sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(config.config_sock_fd);
        return -1;
    }

    if (pthread_mutex_init(&config.m, NULL) != 0) {
        close(config.config_sock_fd);
        return -1;
    }

    config.is_completed = 0;

    config_task.task_handler = logconfig_receiver;
    config_task.arg = (void*)&config;
    if (worker_init(&config.config_worker, &config_task) < 0) {
        close(config.config_sock_fd);
        return -1;
    }

    return 0;
}

int logconfig_deinit(void) {
    worker_deinit(&config.config_worker);
    pthread_mutex_destroy(&config.m);
    close(config.config_sock_fd);
}

char *logconfig_get_path(void) {
    return config_item.log_path;
}

void logconfig_register_config_changed(logconfig_changed_cb_t cb) {
    if (cb == NULL) {
        return;
    }

    registered_callback = cb;
}

char* logconfig_get_logpath(void) {
    return config_item.log_path;
}