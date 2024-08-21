#ifndef COMMON_H
#define COMMON_H

#define LOGRECV_SOCKET_PATH "/tmp/log_service_socket"
#define CONFIG_SOCKET_PATH  "/tmp/log_config_socket"


#define CONFIG_LOG_PATH         1
#define CONFIG_MAX              2

#define CONFIG_MSG_MAX  1024


typedef struct config_msg_t {
    int  config_id;
    char data[CONFIG_MSG_MAX];
} config_msg_t;

#endif