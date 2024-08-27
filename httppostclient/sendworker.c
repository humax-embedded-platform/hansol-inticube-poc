#include "sendworker.h"
#include "httpclient.h"
#include "dbclient.h"
#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define REQUEST_FAIL_COUNT_MAX_PER_ENTRY  20
#define REQUEST_FAIL_RETRY_MAX  5

static task_t sendtask;

static int retry_list_add_exits_item(void* curr, void* new) {
    failure_request_t* req = (failure_request_t*)curr;
    hostinfor_t* failure_host = (hostinfor_t*)new;

    if (memcmp((void*)&req->host, (void*)failure_host, sizeof(hostinfor_t)) == 0) {
        req->failure_count++;
        if(req->failure_count >= REQUEST_FAIL_COUNT_MAX_PER_ENTRY) {
            return 0;
        }else {
            return 1;
        }
    }

    return 0;
}

static void sendworker_add_to_retry_list(linklist_t* list, hostinfor_t host) {
    if(list == NULL) {
        return;
    }

    if(linklist_find_and_update(list, retry_list_add_exits_item, (void*)&host) == 0) {
        failure_request_t failure_item;
        failure_item.host = host;
        failure_item.failure_count = 1;
        failure_item.retry_count = 0;
        node_t* node = (node_t*)malloc(sizeof(node_t));
        linklist_node_init(node, (void*)&failure_item, sizeof(failure_request_t));
        linklist_add(list, node);
    }
}

static void sendworker_task_handler(void* arg) {
    sendworker_t* sw = (sendworker_t*)arg;
    hostinfor_t host;
    httpclient_t client;
    int remain_request = 0;
    int send_status = -1;
    static int sendworker_wait_send_retry_count = 0;

    while (1)
    {
        if(recvworker_waitlist_size(&sw->rev_worker) >= MAX_HANDLE_REQUEST_PER_TIME) {
            usleep(10*1000);
            continue;
        }

        pthread_mutex_lock(&sw->m);
        remain_request = sw->request_count--;
        if(remain_request <= 0 ) {
            sendworker_wait_send_retry_count++;
        }
        pthread_mutex_unlock(&sw->m);

        if(remain_request > 0) {
            if(dbclient_gethost(sw->hostdb, &host)) {
                break;
            }

            send_status = -1;
            if (httpclient_init(&client, host) == 0) {
                if (httpclient_send_post_msg(&client, sw->msg->msg) == 0) {
                    send_status = 0;
                }
            }

            if(send_status < 0) {
                sendworker_add_to_retry_list(&sw->failure_list,host);
                continue;
            }

            recvworker_add_to_waitlist(&sw->rev_worker, client, sw->msg);
        } else {
            if(sendworker_wait_send_retry_count < MAX_SEND_WORKER) {
                continue;
            }

            if(linklist_get_size(&sw->failure_list) <= 0) {
                break;
            } else {
                node_t* retry_item = linklist_remove_from_tail(&sw->failure_list);
                if(retry_item == NULL) {
                    continue;
                }
    
                failure_request_t* req = (failure_request_t*)retry_item->data;
                if(req->retry_count < REQUEST_FAIL_RETRY_MAX) {
                    req->retry_count++;
                } else {
                    linklist_node_deinit(retry_item);
                    continue;
                }

                send_status = -1;
                if (httpclient_init(&client, req->host) == 0) {
                    if (httpclient_send_post_msg(&client, sw->msg->msg) == 0) {
                        send_status = 0;
                    }
                }

                if(send_status < 0) {
                    linklist_add(&sw->failure_list, retry_item);
                    continue;
                } else {
                    req->retry_count = 0;
                    req->failure_count--;
                    if(req->failure_count < 0) {
                        linklist_node_deinit(retry_item);
                    } else {
                        linklist_add(&sw->failure_list, retry_item);                        
                    }
                }

                recvworker_add_to_waitlist(&sw->rev_worker, client, sw->msg);
            }
        }

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

    if (pthread_mutex_init(&sw->m, NULL) != 0) {
        perror("Mutex destruction failed");
        return -1;
    }

    linklist_init(&sw->failure_list);

    sendtask.task_handler = sendworker_task_handler;
    sendtask.arg = sw;

    for (int i = 0; i < MAX_SEND_WORKER; ++i) {
        if (worker_init(&sw->workers[i], &sendtask) != 0) {
            printf("sendworker_init: worker_init failed for worker %d\n", i);
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
        printf("sendworker_deinit: sw is NULL\n");
        return;
    }

    for (int i = 0; i < MAX_SEND_WORKER; ++i) {
        worker_deinit(&sw->workers[i]);
    }

    linklist_deinit(&sw->failure_list);
    pthread_mutex_destroy(&sw->m);
    recvworker_set_completed(&sw->rev_worker);
    recvworker_deinit(&sw->rev_worker);
}
