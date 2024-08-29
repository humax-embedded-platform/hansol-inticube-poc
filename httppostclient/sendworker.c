#include "sendworker.h"
#include "httpclient.h"
#include "dbclient.h"
#include "message.h"
#include "userdbg.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "report.h"

#define REQUEST_FAIL_COUNT_MAX_PER_ENTRY  20
#define REQUEST_FAIL_RETRY_MAX  5

static task_t sendtask;
static int    total_request_failure = 0;

static int retry_list_add_exits_item(void* curr, void* new) {
    failure_request_t* req = (failure_request_t*)curr;
    hostinfor_t* failure_host = (hostinfor_t*)new;

    if (memcmp((void*)&req->host, (void*)failure_host, sizeof(hostinfor_t)) == 0) {
        if(req->failure_count >= REQUEST_FAIL_COUNT_MAX_PER_ENTRY) {
            return 0;
        }else {
            req->failure_count++;
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
    static int sendworker_ready_retry_count = 0;

    while (1)
    {
        if(clientmanager_is_init_client_completed(sw->client_mgr) == 0) {
            if(clientmanager_get_ready(sw->client_mgr,&client) == 0) {
                int send_retry = 0;
                int send_status = -1;
                while (send_retry <= 100)
                {
                    if (httpclient_send_post_msg(&client, sw->msg->msg) == 0) {
                        send_status = 0;
                        break;
                    } else {
                        send_retry++;
                        usleep(10*1000);
                    }
                }
                
                
                if(send_status < 0) {
                    httpclient_deinit(&client);
                    sendworker_add_to_retry_list(&sw->failure_list,client.host);
                    continue;
                }

                recvworker_add_to_waitlist(&sw->rev_worker, client, sw->msg);
            }
        } else {
            pthread_mutex_lock(&sw->m);
            sendworker_ready_retry_count++;
            if(sendworker_ready_retry_count < MAX_SEND_WORKER) {
                pthread_mutex_unlock(&sw->m);
                continue;
            }

            pthread_mutex_unlock(&sw->m);
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
                    report_add_req_failure(req->failure_count);
                    linklist_node_deinit(retry_item);
                    continue;
                }
                int send_status = -1;
                if (httpclient_init(&client, req->host) == 0) {
                    if (httpclient_send_post_msg(&client, sw->msg->msg) == 0) {
                        send_status = 0;
                    } else {
                        httpclient_deinit(&client);
                    }
                }

                if(send_status < 0) {
                    linklist_add(&sw->failure_list, retry_item);
                    continue;
                } else {
                    req->retry_count = 0;
                    req->failure_count--;
                    if(req->failure_count <= 0) {
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


int sendworker_set_msg(sendworker_t* sw, usermsg_t* msg) {
    sw->msg = msg;
}

int sendworker_set_clientmanager(sendworker_t* sw, clientmanager_t *mgr) {
    sw->client_mgr = mgr;
}
int sendworker_init(sendworker_t* sw) {
    if (sw == NULL || sw->msg == NULL || sw->client_mgr == NULL) {
        LOG_DBG("sendworker_init: Invalid arguments\n");
        return -1;
    }

    if (recvworker_init(&sw->rev_worker) != 0) {
        LOG_DBG("sendworker_init: recvworker_init failed\n");
        return -1;
    }

    if (pthread_mutex_init(&sw->m, NULL) != 0) {
        LOG_DBG("sendworker_init: Mutex destruction failed");
        return -1;
    }

    linklist_init(&sw->failure_list);
    sendtask.task_handler = sendworker_task_handler;
    sendtask.arg = sw;

    for (int i = 0; i < MAX_SEND_WORKER; ++i) {
        if (worker_init(&sw->workers[i], &sendtask) != 0) {
            LOG_DBG("sendworker_init: worker_init failed for worker %d\n", i);
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
        LOG_DBG("sendworker_deinit: sw is NULL\n");
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
