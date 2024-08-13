#include "sendworker.h"
#include "httpclient.h"
#include "dbclient.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>

static task_t sendtask;

static void sendworker_task_handler(void* arg) {
    sendworker_t* sw = (sendworker_t*)arg;
    hostinfor_t ip;
    httpclient_t client;

    while (1)
    {
        int ret = dbclient_gethost(sw->hostdb, &ip);
        if(ret) {
            break;
        }

        httpclient_init(&client, ip);
        httpclient_send_post_msg(&client, "heello");
    }
}

int sendworker_init(sendworker_t* sw, db_type_t* hostdb) {
    if (sw == NULL) {
        return -1;
    }

    sw->hostdb = hostdb;
    sendtask.task_handler = sendworker_task_handler;
    sendtask.arg = NULL;

    for (int i = 0; i < MAX_SEND_WORKER; ++i) {
        if (worker_init(&sw->workers[i], &sendtask) != 0) {
            for (int j = 0; j < i; ++j) {
                worker_deinit(&sw->workers[j]);
            }
            return -1;
        }
    }

    return 0;
}

void sendworker_deinit(sendworker_t* sw) {
    if (sw == NULL) {
        return;
    }

    for (int i = 0; i < MAX_SEND_WORKER; ++i) {
        worker_deinit(&sw->workers[i]);
    }
}
