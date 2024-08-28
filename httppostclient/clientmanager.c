#include "clientmanager.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define MAX_EVENTS 10
#define CLIENT_CONNECT_RETRY_MAX 3

static task_t connection_init_task;
static task_t connection_monitor_task;

static void clientmanager_wait_ready(clientmanager_t* mgr, httpclient_t client) {
    if (!mgr) return;
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client.sockfd;

    if (epoll_ctl(mgr->wait_ready_list_fd, EPOLL_CTL_ADD, client.sockfd, &event) == -1) {
        perror("epoll_ctl");

    }

    client_init_info_t clientinfo;
    clientinfo.client = client;
    clientinfo.retry_count = 0;
    clientinfo.init_time = time(NULL);
    node_t* node = (node_t*)malloc(sizeof(node_t));
    linklist_node_init(node,(void*)&clientinfo, sizeof(client_init_info_t));
    linkedlist_add(&mgr->wait_ready_list, node);
}

static void client_wait_reconnect(clientmanager_t* mgr, httpclient_t client) {
    client_init_info_t clientinfo;
    clientinfo.client = client;
    clientinfo.retry_count = 0;
    clientinfo.init_time = time(NULL);
    node_t* node = (node_t*)malloc(sizeof(node_t));
    linklist_node_init(node,(void*)&clientinfo, sizeof(client_init_info_t));
    linkedlist_add(&mgr->reconnect_list, node);
}

static int clientmanager_cmp_client(void* new, void* curr) {
    client_init_info_t* clientinfo = (client_init_info_t*)new;
    int *sockfd = *(int*)curr;

    if(clientinfo->client.sockfd == *sockfd) {
        return 1;
    }

    return 0;
}

static int clientmanager_check_client_ready_timeout_cmp(void *current, void* input) {

    if(current == NULL || input == NULL) {
        return 0;
    }

    int* timeout = (int*)input;
    client_init_info_t* clientinfo = (client_init_info_t*)current;

    time_t current_time = time(NULL);
    if (difftime(current_time, clientinfo->init_time) >= *timeout) {
        clientinfo->retry_count++;
        if (clientinfo->retry_count > CLIENT_CONNECT_RETRY_MAX) {
            return 1;
        }
    }

    return 0;
}

static int clientmanager_check_client_connect_retry_cmp(void *current, void* input) {

    if(current == NULL) {
        return 0;
    }

    client_init_info_t* clientinfo = (client_init_info_t*)current;

    time_t current_time = time(NULL);

    clientinfo->retry_count++;
    if (clientinfo->retry_count > CLIENT_CONNECT_RETRY_MAX) {
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

    while (1)
    {
        pthread_mutex_lock(&mgr->m);
        remain_request = mgr->request_count--;
        wait_ready_list_size = linklist_get_size(&mgr->wait_ready_list);
        reconnect_list_size = linklist_get_size(&mgr->reconnect_list);
        pthread_mutex_unlock(&mgr->m);

        if(remain_request <= 0 && wait_ready_list_size <= 0 && reconnect_list_size <= 0) {
            break;
        }

        if( wait_ready_list_size> 0 || reconnect_list_size > 0) {
            if(wait_ready_list_size > 0) {
                node_t* ready_timeout_node = linklist_find_and_remove(&mgr->wait_ready_list,clientmanager_check_client_ready_timeout_cmp,(void*)&client_ready_time_out, 0);
            }

            if(wait_ready_list_size > 0) {
                node_t* connect_timeout_node = linklist_find_and_remove(&mgr->reconnect_list,clientmanager_check_client_connect_retry_cmp,0, 0);
                linklist_node_deinit(connect_timeout_node);
            }

            sleep(1000*1000);
        }

        if (httpclient_init(&client, host) == 0) {
            clientmanager_wait_ready(mgr,client);
        } else {
            client_wait_reconnect(mgr,client);
        }
    }
    
}

static void clientmanager_connection_monitor_handler(void* args) {
    clientmanager_t* mgr = (clientmanager_t*)args;
    struct epoll_event events[MAX_EVENTS];
    
    while (1) {
        int nfds = epoll_wait(mgr->wait_ready_list_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            int ready_fd = events[i].data.fd;

            node_t* node = linklist_find_and_remove(&mgr->wait_ready_list,clientmanager_cmp_client,(void*)&ready_fd, 0);
            if(node != NULL) {
                linklist_add(&mgr->wait_ready_list, node);

                if (epoll_ctl(mgr->wait_ready_list_fd, EPOLL_CTL_DEL, ready_fd, NULL) == -1) {
                     LOG_DBG("epoll_ctl: EPOLL_CTL_DEL");
                }
            }
        }
    }
}

void clientmanager_init(clientmanager_t* mgr) {
    if (!mgr) return;

    mgr->wait_ready_list_fd = epoll_create1(0);
    if (mgr->wait_ready_list_fd == -1) {
        LOG_DBG("epoll_create1");
        return;
    }

    if (pthread_mutex_init(&mgr->m, NULL) != 0) {
        close(mgr->wait_ready_list_fd);
        LOG_DBG("sendworker_init: Mutex destruction failed");
        return -1;
    }

    connection_monitor_task.task_handler = clientmanager_connection_monitor_handler;
    connection_monitor_task.arg = (void*)mgr;
    if(worker_init(&mgr->connection_monitor,&connection_monitor_task) < 0) {
        close(mgr->wait_ready_list_fd);
        pthread_attr_destroy(&mgr->m);
        return -1;
    }

    connection_init_task.task_handler = clientmanager_connection_init_handler;
    connection_init_task.arg = (void*)mgr;
    if(worker_init(&mgr->connection_init,&connection_init_task) < 0) {
        close(mgr->wait_ready_list_fd);
        pthread_attr_destroy(&mgr->m);
        return -1;
    }

    // Initialize linked lists
    linkedlist_init(&mgr->wait_ready_list);
    linkedlist_init(&mgr->ready_list);
    linkedlist_init(&mgr->reconnect_list);
}

void clientmanager_deinit(clientmanager_t* mgr) {
    if (!mgr) return;

    worker_deinit(&mgr->connection_init);
    worker_deinit(&mgr->connection_monitor);
    close(mgr->wait_ready_list_fd);

    linkedlist_deinit(&mgr->wait_ready_list);
    linkedlist_deinit(&mgr->ready_list);
    linkedlist_deinit(&mgr->reconnect_list);
}

static void clientmanager_wait_ready(clientmanager_t* mgr, httpclient_t client) {
    if (!mgr) return;
    // Prepare epoll event
    struct epoll_event event;
    event.events = EPOLLIN; // Monitor for read readiness
    event.data.fd = client.sockfd;

    if (epoll_ctl(mgr->wait_ready_list_fd, EPOLL_CTL_ADD, client.sockfd, &event) == -1) {
        perror("epoll_ctl");
        // Handle error if needed
    }

    node_t* node = (node_t*)malloc(sizeof(node_t));
    linklist_node_init(node,(void*)&client, sizeof(httpclient_t));
    linkedlist_add(&mgr->wait_ready_list, node);
}

int  clientmanager_get_ready(clientmanager_t* mgr, httpclient_t* client) {

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