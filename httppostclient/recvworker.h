#ifndef RECWORKER_H
#define RECWORKER_H

#include "worker.h"
#include "httpclient.h"
#include "linkedlist.h"
#include "message.h"
#include "report.h"
#include <stdatomic.h>
#include <time.h>

#define MAX_RESPOND_PER_TIME  5
#define RECV_RESPOND_TIMEOUT 5

#define REQUEST_WAIT_RESP_TIMEOUT_ERR   28

typedef struct http_resp_t {
    time_t send_time;
    httpclient_t client;
    usermsg_t* sendmsg;
} http_resp_t;

typedef struct recvworker {
    int epoll_fd;
    worker_t recv_thread;
    worker_t timeout_thread;
    atomic_int is_completed;
    linklist_t* wait_resp_list;
    report_t report;
} recvworker_t;

int  recvworker_init(recvworker_t* rw);
void recvworker_deinit(recvworker_t* rw);
int  recvworker_add_to_waitlist(recvworker_t* rw, httpclient_t client, usermsg_t* sendmsg);
void recvworker_set_completed(recvworker_t* rw);
size_t recvworker_waitlist_size(recvworker_t* rw);

#endif // RECWORKER_H
