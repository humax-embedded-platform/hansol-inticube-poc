#ifndef RECWORKER_H
#define RECWORKER_H

#include "worker.h"
#include "httpclient.h"
#include "linkedlist.h"
#include "common.h"

#include <stdatomic.h>
#include <time.h>

#define MAX_RESPOND_PER_TIME  10
#define RECV_RESPOND_TIMEOUT 5

typedef struct http_resp_t {
    time_t start_time;
    httpclient_t client;
} http_resp_t;

typedef struct recvworker {
    int epoll_fd;
    worker_t recv_thread;
    worker_t timeout_thread;
    atomic_int is_completed;
    linklist_t* wait_resp_list;
} recvworker_t;

int  recvworker_init(recvworker_t* rw);
void recvworker_deinit(recvworker_t* rw);
int  recvworker_add_to_waitlist(recvworker_t* rw, httpclient_t client);
void recvworker_set_completed(recvworker_t* rw);

#endif // RECWORKER_H
