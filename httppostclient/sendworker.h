#ifndef SENDWORKER_H
#define SENDWORKER_H

#include "worker.h"
#include "dbclient.h"
#include "message.h"
#include <stdatomic.h>

#define MAX_SEND_WORKER 10

typedef struct sendworker {
    db_type_t*  hostdb;
    worker_t    workers[MAX_SEND_WORKER];
} sendworker_t;

int sendworker_init(sendworker_t* sw, db_type_t* hostdb);
void sendworker_deinit(sendworker_t* sw);

#endif // SENDWORKER_H
