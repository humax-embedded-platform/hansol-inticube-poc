#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#define IPV4_ADRESS_SIZE 16
#define DOMAIN_ADRESS_SIZE 255

#define HTTPS_DEFAULT_PORT 443
#define HTTP_DEFAILT_PORT  80

#define HOST_IPV4_TYPE    1
#define HOST_DOMAIN_TYPE  2

#define HTTP_PROTOCOL     1
#define HTTPS_PROTOCOL    2

typedef struct hostinfor_t {
    union {
        char ip[IPV4_ADRESS_SIZE];
        char domain[DOMAIN_ADRESS_SIZE];
    } adress;
    char adress_type;
    char protocol;
    int  port;
} hostinfor_t;

#endif