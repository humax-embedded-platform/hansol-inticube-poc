#include "sendworker.h"
#include "httpclient.h"
#include "dbclient.h"
#include "common.h"
#include "message.h"
#include <stdio.h>
#include <stdlib.h>

static task_t sendtask;

static void sendworker_task_handler(void* arg) {
    sendworker_t* sw = (sendworker_t*)arg;
    hostinfor_t ip;
    httpclient_t client;
    static int sequence_number = 0;  // Static to maintain sequence across calls

    printf("sendworker_task_handler: Started\n");

    while (1)
    {
        int ret = dbclient_gethost(sw->hostdb, &ip);
        if(ret) {
            //printf("dbclient_gethost failed with error code %d\n", ret);
            break;
        }

        if (httpclient_init(&client, ip) != 0) {
            printf("httpclient_init failed for ip: %s, port: %d\n", ip.ip, ip.port);
            continue;
        }

        char new_msg[1024];  // Adjust the size as necessary
        snprintf(new_msg, sizeof(new_msg), "%s [Seq: %d]", sw->msg->msg, sequence_number);

        // Increment sequence number for the next message
        sequence_number++;

        if (httpclient_send_post_msg(&client, new_msg) != 0) {
            printf("httpclient_send_post_msg failed for ip: %s, port: %d\n", ip.ip, ip.port);
            continue;
        }

        recvworker_add_to_waitlist(&sw->rev_worker, client);
        printf("recvworker_add_to_waitlist: Added client for ip: %s, port: %d\n", ip.ip, ip.port);
    }

    printf("sendworker_task_handler: Exiting\n");
}

int sendworker_init(sendworker_t* sw, usermsg_t* msg ,dbclient* hostdb) {
    if (sw == NULL || msg == NULL || hostdb == NULL) {
        printf("sendworker_init: Invalid arguments\n");
        return -1;
    }

    if (recvworker_init(&sw->rev_worker) != 0) {
        printf("sendworker_init: recvworker_init failed\n");
        return -1;
    }

    sw->hostdb = hostdb;
    sw->msg    = msg;

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
        printf("sendworker_deinit: Worker %d deinitialized\n", i);
    }

    printf("sendworker_deinit: Setting recvworker as completed\n");
    recvworker_set_completed(&sw->rev_worker);

    printf("sendworker_deinit: Deinitializing recvworker\n");
    recvworker_deinit(&sw->rev_worker);
}
