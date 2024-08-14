#ifndef SENDWORKER_H
#define SENDWORKER_H

#include "worker.h"
#include "dbclient.h"
#include "message.h"
#include "recvworker.h"
#include <stdatomic.h>

#define MAX_SEND_WORKER 10

typedef struct sendworker {
    dbclient*  hostdb;
    worker_t    workers[MAX_SEND_WORKER];
    recvworker_t rev_worker;
    usermsg_t* msg;
} sendworker_t;

int sendworker_init(sendworker_t* sw, usermsg_t* msg, dbclient* hostdb);
void sendworker_deinit(sendworker_t* sw);

#endif // SENDWORKER_H
