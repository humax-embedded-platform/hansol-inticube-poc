#include "clientmanager.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "report.h"
#include "userdbg.h"
#include <string.h>
#include <stdlib.h>

#define MAX_EVENTS 50
#define CLIENT_CONNECT_RETRY_MAX 200

static task_t connection_init_task;
static task_t connection_monitor_task;
static task_t connection_retry_task;

static void clientmanager_wait_ready(clientmanager_t* mgr, httpclient_t client) {
    if (mgr == NULL) return;
    struct epoll_event event;
    event.events = EPOLLOUT | EPOLLONESHOT;
    event.data.fd = client.sockfd;

    if (epoll_ctl(mgr->wait_ready_list_fd, EPOLL_CTL_ADD, client.sockfd, &event) == -1) {
        LOG_DBG("clientmanager_wait_ready add error\n");
    }

    client_init_info_t clientinfo;
    clientinfo.client = client;
    clientinfo.retry_count = 0;
    gettimeofday(&clientinfo.init_time, NULL);
    node_t* node = (node_t*)malloc(sizeof(node_t));
    linklist_node_init(node,(void*)&clientinfo, sizeof(client_init_info_t));
    linklist_add(&mgr->wait_ready_list, node);
}

static void client_wait_reconnect(clientmanager_t* mgr, httpclient_t client) {
    client_init_info_t clientinfo;
    clientinfo.client = client;
    clientinfo.retry_count = 0;
    gettimeofday(&clientinfo.init_time, NULL);
    node_t* node = (node_t*)malloc(sizeof(node_t));
    linklist_node_init(node,(void*)&clientinfo, sizeof(client_init_info_t));
    linklist_add(&mgr->reconnect_list, node);
}


static int clientmanager_cmp_client(void* curr, void* input) {
    client_init_info_t* clientinfo = (client_init_info_t*)curr;
    int sockfd = *(int*)input;

    if(clientinfo->client.sockfd == sockfd) {
        return 1;
    }

    return 0;
}

static int clientmanager_check_client_timeout_cmp(void *current, void* input) {
    if(current == NULL || input == NULL) {
        return 0;
    }

    int* timeout = (int*)input;
    client_init_info_t* clientinfo = (client_init_info_t*)current;

    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    long seconds_diff = current_time.tv_sec - clientinfo->init_time.tv_sec;
    long microseconds_diff = current_time.tv_usec - clientinfo->init_time.tv_usec;

    long total_diff_ms = (seconds_diff * 1000) + (microseconds_diff / 1000);

    if (total_diff_ms >= (*timeout)) {
        return 1;
    }

    return 0;
}


static void clientmanager_connection_init_handler(void* args) {
    clientmanager_t* mgr = (clientmanager_t*)args;
    int remain_request = 0;
    int wait_ready_list_size = 0;
    int reconnect_list_size = 0;
    httpclient_t client;
    hostinfor_t host;
    int client_ready_time_out =2;
    int count = 0;

    while (1)
    {
        pthread_mutex_lock(&mgr->m);

        mgr->request_count --;
        if(mgr->request_count < 0) {
            pthread_mutex_unlock(&mgr->m);
            break;
        }

        remain_request= mgr->request_count;
        pthread_mutex_unlock(&mgr->m);

        if(remain_request > 0) {
            if(dbclient_gethost(mgr->db, &host) < 0) {
                pthread_mutex_lock(&mgr->m);
                mgr->request_count = 0;
                pthread_mutex_unlock(&mgr->m);
                break;
            }
    
            if (httpclient_init(&client, host) == 0) {
                clientmanager_wait_ready(mgr,client);
            } else {
                client_wait_reconnect(mgr,client);
            }
        }
    }
}

static void clientmanager_check_reconnect_retry(clientmanager_t* mgr) {
    int connection_retry_timeout = 100;
    while(1) {
        node_t* connect_retry_node = linklist_find_and_remove(&mgr->reconnect_list,clientmanager_check_client_timeout_cmp,(void*)&connection_retry_timeout, 0);
        if(connect_retry_node != NULL) {
            client_init_info_t* clientinfo = (client_init_info_t*)connect_retry_node->data;
            clientinfo->retry_count++;
            if (httpclient_init(&clientinfo->client, clientinfo->client.host) == 0) {
                clientmanager_wait_ready(mgr,clientinfo->client);
                linklist_node_deinit(connect_retry_node);
            } else {
                gettimeofday(&clientinfo->init_time, NULL);
                if(clientinfo->retry_count > 10) {
                    report_add_req_failure(1);
                    linklist_node_deinit(connect_retry_node);
                } else {
                    linklist_add(&mgr->reconnect_list, connect_retry_node);
                }
            }
        } else {
            break;
        }
    } 
}

static void clientmanager_check_client_ready(clientmanager_t* mgr) {
    int connection_ready_timeout = 500;
    while(1) {
        node_t* connect_timeout_node = linklist_find_and_remove(&mgr->wait_ready_list,clientmanager_check_client_timeout_cmp,(void*)&connection_ready_timeout, 0);
        if(connect_timeout_node != NULL) {
            client_init_info_t* clientinfo = (client_init_info_t*)connect_timeout_node->data;
            if(clientinfo->client.sockfd > 0) {
                epoll_ctl(mgr->wait_ready_list_fd, EPOLL_CTL_DEL, clientinfo->client.sockfd, NULL);
                close(clientinfo->client.sockfd);
            }

            clientinfo->retry_count++;
            if(clientinfo->retry_count > 10) {
                report_add_req_failure(1);
                linklist_node_deinit(connect_timeout_node);
            } else {
                clientinfo->client.sockfd = 0;
                httpclient_init(&clientinfo->client, clientinfo->client.host); 
                gettimeofday(&clientinfo->init_time,NULL);   
                linklist_add(&mgr->wait_ready_list, connect_timeout_node);     
            }
        } else {
            break;
        }
    }    
}

static void clientmanager_connection_monitor_handler(void* args) {
    clientmanager_t* mgr = (clientmanager_t*)args;
    struct epoll_event events[MAX_EVENTS];
    int remain_request = 0;
    int wait_ready_list_size = 0;
    int reconnect_list_size = 0;
    while (1) {
        pthread_mutex_lock(&mgr->m);
        remain_request = mgr->request_count;
        wait_ready_list_size = linklist_get_size(&mgr->wait_ready_list);
        reconnect_list_size = linklist_get_size(&mgr->reconnect_list);
        if(remain_request <= 0 && wait_ready_list_size <= 0 && reconnect_list_size <= 0) {
            mgr->is_completed = 1;
            pthread_mutex_unlock(&mgr->m);
            break;
        }
        pthread_mutex_unlock(&mgr->m);
    
        int nfds = epoll_wait(mgr->wait_ready_list_fd, events, MAX_EVENTS, 100);
        if (nfds == -1) {
            LOG_DBG("epoll_wait");
            break;
        }

        if(nfds == 0) {

            continue;
        }

        for (int i = 0; i < nfds; i++) {
            int ready_fd = events[i].data.fd;

            node_t* node = linklist_find_and_remove(&mgr->wait_ready_list,clientmanager_cmp_client,(void*)&ready_fd, 0);
            if(node != NULL) {
                linklist_add(&mgr->ready_list, node);
            }
        }
    }
}

static void clientmanager_connection_retry_handler(void* args) {
    clientmanager_t* mgr = (clientmanager_t*)args;
    int remain_request = 0;
    int wait_ready_list_size = 0;
    int reconnect_list_size = 0;

    while (1)
    {
        pthread_mutex_lock(&mgr->m);
        remain_request = mgr->request_count;
        wait_ready_list_size = linklist_get_size(&mgr->wait_ready_list);
        reconnect_list_size = linklist_get_size(&mgr->reconnect_list);
        if(remain_request <= 0 && wait_ready_list_size <= 0 && reconnect_list_size <= 0) {
            pthread_mutex_unlock(&mgr->m);
            break;
        }
        pthread_mutex_unlock(&mgr->m);
        clientmanager_check_reconnect_retry(mgr);
        clientmanager_check_client_ready(mgr);
        usleep(500*1000);
    }
}

int clientmanager_init(clientmanager_t* mgr) {
    if (mgr ==  NULL || mgr->db == NULL || mgr->request_count <= 0) return -1;

    mgr->wait_ready_list_fd = epoll_create1(0);
    if (mgr->wait_ready_list_fd == -1) {
        LOG_DBG("epoll_create1");
        return -1;
    }

    if (pthread_mutex_init(&mgr->m, NULL) != 0) {
        close(mgr->wait_ready_list_fd);
        LOG_DBG("sendworker_init: Mutex destruction failed");
        return -1;
    }

    mgr->is_completed = 0;

    linklist_init(&mgr->wait_ready_list);
    linklist_init(&mgr->ready_list);
    linklist_init(&mgr->reconnect_list);

    connection_retry_task.task_handler = clientmanager_connection_retry_handler;
    connection_retry_task.arg = (void*)mgr;
    if(worker_init(&mgr->connection_retry,&connection_retry_task) < 0) {
        LOG_DBG("sendworker_init: clientmanager_connection_retry_handler");
        close(mgr->wait_ready_list_fd);
        pthread_mutex_destroy(&mgr->m);
        return -1;
    }

    connection_monitor_task.task_handler = clientmanager_connection_monitor_handler;
    connection_monitor_task.arg = (void*)mgr;
    if(worker_init(&mgr->connection_monitor,&connection_monitor_task) < 0) {
        LOG_DBG("sendworker_init: clientmanager_connection_monitor_handler");
        close(mgr->wait_ready_list_fd);
        pthread_mutex_destroy(&mgr->m);
        return -1;
    }

    connection_init_task.task_handler = clientmanager_connection_init_handler;
    connection_init_task.arg = (void*)mgr;
    if(worker_init(&mgr->connection_init,&connection_init_task) < 0) {
        LOG_DBG("sendworker_init: clientmanager_connection_init_handlerXXX");

        close(mgr->wait_ready_list_fd);
        pthread_mutex_destroy(&mgr->m);
        return -1;
    }

    LOG_DBG("INIT SUCCESS");

    return 0;
}

void clientmanager_deinit(clientmanager_t* mgr) {
    if (!mgr) return;

    worker_deinit(&mgr->connection_init);
    worker_deinit(&mgr->connection_monitor);
    worker_deinit(&mgr->connection_retry);

    pthread_mutex_destroy(&mgr->m);

    linklist_deinit(&mgr->wait_ready_list);
    linklist_deinit(&mgr->ready_list);
    linklist_deinit(&mgr->reconnect_list);
    close(mgr->wait_ready_list_fd);
}

int  clientmanager_get_ready(clientmanager_t* mgr, httpclient_t* client) {
    node_t* node = linklist_remove_from_tail(&mgr->ready_list);
    if(node == NULL) {
        return -1;
    }

    client_init_info_t* clientinfo = (client_init_info_t*)node->data;
    memcpy((void*)client ,(void*)&clientinfo->client,sizeof(httpclient_t));
    LOG_DBG("%s ",clientinfo->client.host.adress.ip);

    linklist_node_deinit(node);


    return 0;
}

int  clientmanager_set_hostdb(clientmanager_t* mgr, dbclient* db) {
    if(mgr != NULL) {
        mgr->db = db;
    }
}

int  clientmanager_set_request_count(clientmanager_t* mgr, int count) {
    if(mgr != NULL) {
        mgr->request_count = count;
    }
}

int  clientmanager_is_init_client_completed(clientmanager_t* mgr) {
    int is_completed = 0;
    pthread_mutex_lock(&mgr->m);
    is_completed = mgr->is_completed;
    pthread_mutex_unlock(&mgr->m);

    return is_completed;
}