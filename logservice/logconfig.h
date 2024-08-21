
#ifndef LOGCONFIG_H
#define LOGCONFIG_H

#include "worker.h"
#include "common.h"
#include <stdatomic.h>

typedef struct logconfig_item_t
{
    char  log_path[1024];
}logconfig_item_t;

typedef struct logconfig_t
{
    int      config_sock_fd;
    worker_t config_worker;
    atomic_int is_completed;
} logconfig_t;

typedef void (*logconfig_handler_t)(const config_msg_t*);
typedef void (*logconfig_changed_cb_t)(int id ,const logconfig_item_t*);

int   logconfig_init(void);
int   logconfig_deinit(void);
char* logconfig_get_logpath(void);
void  logconfig_register_config_changed(logconfig_changed_cb_t cb);

#endif