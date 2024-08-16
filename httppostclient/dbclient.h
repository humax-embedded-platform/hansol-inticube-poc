#ifndef DBCLIENT_H
#define DBCLIENT_H

#include "textdb.h"
#include "common.h"
#include <stdatomic.h>  // Include for atomic operations
#include <pthread.h>   // Include for pthread_mutex_t

typedef enum {
    TEXT_DB,
    INVALID_DB,
} db_type_t;


typedef struct dbclient {
    union {
        textdb_t textdb;  // Currently, the client only supports textdb

    } db;
    
    int current_index;  // Atomic integer for index
    db_type_t type;  // Store the type of database used

    atomic_flag index_lock;  // Spinlock to protect current_index
} dbclient;

int dbclient_init(dbclient* client, db_type_t type, const char* dbpath);

int dbclient_deinit(dbclient* client);

int dbclient_gethost(dbclient* client, hostinfor_t* host);

#endif // DBCLIENT_H
