#ifndef SENDWORKER_H
#define SENDWORKER_H

#include "worker.h"
#include "dbclient.h"
#include "message.h"
#include "recvworker.h"
#include "linkedlist.h"
#include "clientmanager.h"

#include <pthread.h>

#define MAX_SEND_WORKER  10
#define MAX_HANDLE_REQUEST_PER_TIME 10000

typedef struct failure_request_t
{
    int failure_count;
    int retry_count;
    hostinfor_t host;
}failure_request_t;

typedef struct sendworker {
    usermsg_t*   msg;
    worker_t     workers[MAX_SEND_WORKER];
    recvworker_t rev_worker;
    clientmanager_t* client_mgr;
    linklist_t   failure_list;
    pthread_mutex_t m;
} sendworker_t;

int sendworker_set_msg(sendworker_t* sw, usermsg_t* msg);
int sendworker_set_clientmanager(sendworker_t* sw, clientmanager_t *mgr);
int sendworker_init(sendworker_t* sw);
void sendworker_deinit(sendworker_t* sw);

#endif // SENDWORKER_H
