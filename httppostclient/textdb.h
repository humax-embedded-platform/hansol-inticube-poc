#ifndef TEXTDB_H
#define TEXTDB_H

#include <stddef.h>
#include <pthread.h> 
#include "httpclient.h"

typedef struct {
    char*  data;
    size_t size;
    int    read_offset;
    pthread_mutex_t read_lock;
} textdb_t;


int textdb_init(textdb_t* db, const char* dbpath);

int textdb_deinit(textdb_t* textdb);

int textdb_gethost(textdb_t* textdb, hostinfor_t* host);

#endif // TEXTDB_H
