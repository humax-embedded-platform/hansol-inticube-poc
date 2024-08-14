#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#define IPV4_ADRESS_SIZE 16

typedef struct hostinfor_t {
    char ip[IPV4_ADRESS_SIZE];
    int  port;
} hostinfor_t;

typedef struct {
    char*  data;
    size_t size;
} textdb_t;

#endif