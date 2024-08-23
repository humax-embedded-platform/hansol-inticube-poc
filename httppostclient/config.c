#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h>

#define BUFFER_SIZE 1024

static config_t config_server;
static config_item_t config_items[CONFIG_LIST_MAX];
static config_registered_list_t config_register[CONFIG_LIST_MAX];
static task_t config_task;
#define CONFIG_SOCKET_PATH "/tmp/config_server.sock"

static void config_init_items_default() {

    for (int i = 0; i < CONFIG_LIST_MAX; i++) {
        memset(config_items[i].config_id, 0, CONFIG_ID_MAX_LEN);
        memset(config_items[i].config_value, 0, CONFIG_VALUE_MAX_LEN);
    }

    strncpy(config_items[0].config_id, CONFIG_LOG_FOLDER_PATH_ID, CONFIG_ID_MAX_LEN - 1);
    config_items[0].config_id[CONFIG_ID_MAX_LEN - 1] = '\0';
    strncpy(config_items[0].config_value, CONFIG_LOG_FOLDER_PATH_DEFAULT, CONFIG_VALUE_MAX_LEN - 1);
    config_items[0].config_value[CONFIG_VALUE_MAX_LEN - 1] = '\0';

    for (int i = 0; i < CONFIG_LIST_MAX; i++) {
        for (int j = 0; j < CONFIG_CLIENT_MAX; j++) {
            config_register[i].client[j] = NULL;
        }
    }
}

static void config_deinit_items_default() {
    for (int i = 0; i < CONFIG_LIST_MAX; i++) {
        for (int j = 0; j < CONFIG_CLIENT_MAX; j++) {
            if (config_register[i].client[j] != NULL) {
                free(config_register[i].client[j]);
                config_register[i].client[j] = NULL;
            }
        }
    }
}

static void config_changed_notify_client(const char* config_id) {
    int found_index = -1;

    for (int i = 0; i < CONFIG_LIST_MAX; i++) {
        if (strncmp(config_items[i].config_id, config_id, CONFIG_ID_MAX_LEN) == 0) {
            found_index = i;
            break;
        }
    }

    if (found_index == -1) {
        printf("Config ID not found: %s\n", config_id);
        return;
    }

    config_msg_t msg;
    snprintf(msg.cmd, sizeof(msg.cmd), "%s", "config_changed");
    snprintf(msg.cmd_value.config_id, sizeof(msg.cmd_value.config_id), "%s", config_id);
    msg.cmd_value.config_value[0] = '\0';

    for (int i = 0; i < CONFIG_CLIENT_MAX; i++) {
        if (config_register[found_index].client[i] != NULL) {
            struct sockaddr_un* client_addr = (struct sockaddr_un*)config_register[found_index].client[i];

            ssize_t sent_len = sendto(config_server.config_server_sockfd, &msg, sizeof(config_msg_t), 0,
                                      (struct sockaddr*)client_addr, sizeof(struct sockaddr_un));
            
            if (sent_len == -1) {
                perror("sendto");
            } else {
                printf("Notification sent to client for config_id: %s\n", config_id);
            }
        }
    }
}

static int config_register_config(struct sockaddr* client,config_item_t* item) {
    int found_index  = -1;
    int is_registered = -1;

    for (int i = 0; i < CONFIG_LIST_MAX; i++) {
        if (strncmp(config_items[i].config_id, item->config_id, CONFIG_ID_MAX_LEN) == 0) {
            found_index  = i;
            break;
        }
    }

    if (found_index  >= 0) {
        for (int i = 0; i < CONFIG_CLIENT_MAX; i++) {
            if (config_register[found_index].client[i] != NULL &&
                memcmp(config_register[found_index].client[i], client, sizeof(struct sockaddr)) == 0) {
                printf("Client already registered for config_id: %s\n", item->config_id);
                return 0;
            }
        }

        for (int i = 0; i < CONFIG_CLIENT_MAX; i++) {
            if (config_register[found_index].client[i] == NULL) {
                config_register[found_index].client[i] = malloc(sizeof(struct sockaddr));
                if (config_register[found_index].client[i] == NULL) {
                    perror("malloc");
                    return -1;
                }
                memcpy(config_register[found_index].client[i], client, sizeof(struct sockaddr));
                printf("Client added to existing config: %s\n", item->config_id);
                return 0;
            }
        }
    }

    return -1;
}

static int config_unregister_config(struct sockaddr *client, config_item_t *item) {
    int found_index = -1;

    for (int i = 0; i < CONFIG_LIST_MAX; i++) {
        if (strncmp(config_items[i].config_id, item->config_id, CONFIG_ID_MAX_LEN) == 0) {
            found_index = i;
            break;
        }
    }

    if (found_index >= 0) {
        for (int i = 0; i < CONFIG_CLIENT_MAX; i++) {
            if (config_register[found_index].client[i] != NULL &&
                memcmp(config_register[found_index].client[i], client, sizeof(struct sockaddr)) == 0) {
                
                free(config_register[found_index].client[i]);
                config_register[found_index].client[i] = NULL;
                printf("Client unregistered for config_id: %s\n", item->config_id);
                return 0;
            }
        }

        printf("Client not found for config_id: %s\n", item->config_id);
        return -1;
    }

    printf("Config_id not found: %s\n", item->config_id);
    return -1;
}

static void config_task_handler(void* arg) {
    config_t* config_server = (config_t*)arg;
    struct sockaddr_un client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[1024];
    int bytes_received = 0;

    while (1)
    {
        int bytes_received = recvfrom(config_server->config_server_sockfd, buffer, sizeof(buffer) - 1, 0,
                                  (struct sockaddr *)&client_addr, &client_addr_len);
        if (bytes_received <=0 ) {
            perror("recvfrom");
            return;
        }

        buffer[bytes_received] = '\0';

        config_msg_t msg;
        size_t offset = 0;
        memcpy(msg.cmd, buffer + offset, CONFIG_CMD_MAX_LEN - 1);
        msg.cmd[CONFIG_CMD_MAX_LEN - 1] = '\0';
        offset += CONFIG_CMD_MAX_LEN;

        // Copy the config_id
        memcpy(msg.cmd_value.config_id, buffer + offset, CONFIG_ID_MAX_LEN - 1);
        msg.cmd_value.config_id[CONFIG_ID_MAX_LEN - 1] = '\0';  // Ensure null termination
        offset += CONFIG_ID_MAX_LEN;

        // Copy the config_value
        memcpy(msg.cmd_value.config_value, buffer + offset, CONFIG_VALUE_MAX_LEN - 1);
        msg.cmd_value.config_value[CONFIG_VALUE_MAX_LEN - 1] = '\0';

        if (strcmp(msg.cmd, "register_config") == 0) {
            config_register_config((struct sockaddr *)&client_addr, &msg.cmd_value);
            printf("Registering config: ID=%s, Value=%s\n", msg.cmd_value.config_id, msg.cmd_value.config_value);
        } else if (strcmp(msg.cmd, "unregister_config") == 0) {
            config_unregister_config((struct sockaddr *)&client_addr, &msg.cmd_value);
            printf("Unregistering config: ID=%s\n", msg.cmd_value.config_id);
        } else {
            printf("not supported command: %s\n", msg.cmd);
        }
    }
}

void config_init(void) {
    struct sockaddr_un server_addr;
    
    config_server.config_server_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (config_server.config_server_sockfd == -1) {
        perror("socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, CONFIG_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(CONFIG_SOCKET_PATH);

    if (bind(config_server.config_server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return -1;
    }

    config_init_items_default();


    config_task.task_handler = config_task_handler;
    config_task.arg = (void*)&config_server;
    if(worker_init(&config_server.worker,&config_task) < 0 ) {
        close(config_server.config_server_sockfd);
        config_deinit_items_default();
        return -1;
    }

    return 0;
}


void config_deinit(void) {
    worker_deinit(&config_server.worker);
    close(config_server.config_server_sockfd);
    config_deinit_items_default();

    unlink(CONFIG_SOCKET_PATH);
}
