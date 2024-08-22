#include "sendworker.h"
#include "httpclient.h"
#include "dbclient.h"
#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static task_t sendtask;

static void sendworker_task_handler(void* arg) {
    sendworker_t* sw = (sendworker_t*)arg;
    hostinfor_t host;
    httpclient_t client;
    int remain_request = 0;

    while (1)
    {
        if(recvworker_waitlist_size(&sw->rev_worker) >= MAX_HANDLE_REQUEST_PER_TIME) {
            usleep(10*1000);
            continue;
        }

        while (atomic_flag_test_and_set(&sw->request_count_check_flag)) {}

        sw->request_count--;
        if (sw->request_count < 0) {
            atomic_flag_clear(&sw->request_count_check_flag);
            break;
        }

        atomic_flag_clear(&sw->request_count_check_flag);

        if(dbclient_gethost(sw->hostdb, &host)) {
            break;
        }

        if (httpclient_init(&client, host) != 0) {
            continue;
        }

        if (httpclient_send_post_msg(&client, sw->msg->msg) != 0) {
            continue;
        }

        recvworker_add_to_waitlist(&sw->rev_worker, client, sw->msg);
    }
}

int sendworker_set_hostdb(sendworker_t* sw, dbclient* db) {
    sw->hostdb = db;
}
int sendworker_set_msg(sendworker_t* sw, usermsg_t* msg) {
    sw->msg = msg;
}
int sendworker_set_request_count(sendworker_t* sw, int count) {
    sw->request_count = count;
}

int sendworker_init(sendworker_t* sw) {
    if (sw == NULL || sw->msg == NULL || sw->hostdb == NULL) {
        printf("sendworker_init: Invalid arguments\n");
        return -1;
    }

    if (recvworker_init(&sw->rev_worker) != 0) {
        printf("sendworker_init: recvworker_init failed\n");
        return -1;
    }

    sendtask.task_handler = sendworker_task_handler;
    sendtask.arg = sw;  // Pass the sendworker struct to the task handler

    for (int i = 0; i < MAX_SEND_WORKER; ++i) {
        if (worker_init(&sw->workers[i], &sendtask) != 0) {
            printf("sendworker_init: worker_init failed for worker %d\n", i);
            for (int j = 0; j < i; ++j) {
                worker_deinit(&sw->workers[j]);
            }
            return -1;
        }
    }

    printf("sendworker_init: Initialization complete\n");

    return 0;
}

void sendworker_deinit(sendworker_t* sw) {
    if (sw == NULL) {
        printf("sendworker_deinit: sw is NULL\n");
        return;
    }

    for (int i = 0; i < MAX_SEND_WORKER; ++i) {
        worker_deinit(&sw->workers[i]);
    }

    recvworker_set_completed(&sw->rev_worker);
    recvworker_deinit(&sw->rev_worker);
}
