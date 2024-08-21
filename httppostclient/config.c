#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "config.h"
#include "log.h"
#include "common.h"

static config_item_t config;

void config_init(void) {
    config.request_count = 0;
}

void config_deinit(void) {
    if (config.msg_file) {
        free(config.msg_file);
        config.msg_file = NULL;
    }

    if (config.host_file) {
        free(config.host_file);
        config.host_file = NULL;
    }

    if (config.log_folder) {
        free(config.log_folder);
        config.log_folder = NULL;
    }

    config.request_count = 0;
}

const char* config_get_msg_file() {
    return config.msg_file;
}

const char* config_get_host_file() {
    return config.host_file;
}

const char* config_get_log_folder() {
    return config.log_folder;
}

const int config_get_request_count() {
    return config.request_count;
}

int config_set_msg_file(const char* msg_file) {
    if (access(msg_file, F_OK) != 0) {
        perror("File does not exist");
        return -1;
    }

    if (config.msg_file) {
        free(config.msg_file);
    }

    config.msg_file = strdup(msg_file);
    if (config.msg_file == NULL) {
        perror("Failed to allocate memory for msg_file");
        return -1;
    }

    return 0;
}

int config_set_host_file(const char* host_file) {
    if (access(host_file, F_OK) != 0) {
        perror("File does not exist");
        return -1;
    }

    if (config.host_file) {
        free(config.host_file);
    }

    config.host_file = strdup(host_file);
    if (config.host_file == NULL) {
        perror("Failed to allocate memory for msg_file");
        return -1;
    }

    return 0;
}

int config_set_log_folder(const char* log_folder) {
    if (access(log_folder, F_OK) != 0) {
        if (mkdir(log_folder, 0755) != 0) {
            perror("Failed to create log folder");
            return -1;
        }
    }

    if (config.log_folder) {
        free(config.log_folder);
    }

    config.log_folder = strdup(log_folder);
    if (config.log_folder == NULL) {
        perror("Failed to allocate memory for log_folder");
        return -1;
    }

    return 0;
}

int config_set_request_count(int request_count) {
    if (request_count <= 0) {
        printf("request number need > 0\n");
        return -1;
    }
    config.request_count = request_count;
    return 0;
}
