#ifndef CONFIG_H
#define CONFIG_H

#include "worker.h"

#define CONFIG_ID_MAX_LEN       24
#define CONFIG_VALUE_MAX_LEN    1024
#define CONFIG_LIST_MAX         10
#define CONFIG_CLIENT_MAX       10
#define CONFIG_CMD_MAX_LEN      24

#define CONFIG_CMD_REGISTER   "register_config"   // client -> server
#define CONFIG_CMD_UNREGISTER "unregister_config" // client -> server
#define CONFIG_DATA_CHANGED   "config_changed"    // server -> client

#define CONFIG_LOG_FOLDER_PATH_ID       "config_log_folder_path"
#define CONFIG_LOG_FOLDER_PATH_DEFAULT  "./"
#define CONFIG_REQUEST_NUMBER_ID   "config_request_number"
#define CONFIG_HOST_FILE_PATH_ID   "config_rq_number_path"
#define CONFIG_INTPUT_FILE_PATH_ID "config_input_file_path"

typedef struct config_item_t {
    char  config_id[CONFIG_ID_MAX_LEN];
    char  config_value[CONFIG_VALUE_MAX_LEN];
} config_item_t;

typedef struct config_registered_list_t {
    char  config_id[CONFIG_ID_MAX_LEN];
    struct sockaddr* client[CONFIG_CLIENT_MAX];
} config_registered_list_t;

typedef struct config_t {
    worker_t worker;
    int config_server_sockfd;
} config_t;

typedef struct config_msg_t {
    char cmd[20];
    config_item_t cmd_value;
} config_msg_t;

void config_init(void);
void config_deinit(void);

const char* config_get_msg_file();
const char* config_get_host_file();
const char* config_get_log_folder();
const int   config_get_request_count();

int config_set_msg_file(const char* msg_file);
int config_set_host_file(const char* host_file);
int config_set_log_folder(const char* log_folder);
int config_set_request_count(int request_count);

#endif
