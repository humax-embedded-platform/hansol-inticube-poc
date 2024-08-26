#ifndef SENDWORKER_H
#define SENDWORKER_H

#include "worker.h"
#include "dbclient.h"
#include "message.h"
#include "recvworker.h"
#include <pthread.h>

#define MAX_SEND_WORKER  10
#define MAX_HANDLE_REQUEST_PER_TIME 10000

typedef struct sendworker {
    dbclient*    hostdb;
    usermsg_t*   msg;
    worker_t     workers[MAX_SEND_WORKER];
    recvworker_t rev_worker;
    int          request_count;
    pthread_mutex_t m;
} sendworker_t;

int sendworker_set_hostdb(sendworker_t* sw, dbclient* db);
int sendworker_set_msg(sendworker_t* sw, usermsg_t* msg);
int sendworker_set_request_count(sendworker_t* sw, int cound);
int sendworker_init(sendworker_t* sw);
void sendworker_deinit(sendworker_t* sw);

#endif // SENDWORKER_H
