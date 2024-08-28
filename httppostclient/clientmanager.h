#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include "httpclient.h"
#include "linkedlist.h"
#include "worker.h"
#include "dbclient.h"
#include <pthread.h>

typedef struct client_init_info_t {
    httpclient_t client;
    time_t init_time;
    int retry_count;
}client_init_info_t;

typedef struct clientmanager_t{
    int wait_ready_list_fd;
    linklist_t wait_ready_list;
    linklist_t ready_list;
    linklist_t reconnect_list;
    worker_t connection_init;
    worker_t connection_retry;
    worker_t connection_monitor;
    dbclient* db;
    int request_count;
    int total_hander_request;
    pthread_mutex_t m;
    int is_completed;
} clientmanager_t;

int clientmanager_init(clientmanager_t* mgr);
void clientmanager_deinit(clientmanager_t* mgr);
int  clientmanager_get_ready(clientmanager_t* mgr, httpclient_t* client);
int  clientmanager_set_hostdb(clientmanager_t* mgr, dbclient* db);
int  clientmanager_set_request_count(clientmanager_t* mgr, int count);
int  clientmanager_is_init_client_completed(clientmanager_t* mgr);
#endif