#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <stddef.h>

#define IPV4_ADRESS_SIZE 16
#define DOMAIN_ADRESS_SIZE 255

#define HTTPS_DEFAULT_PORT 443
#define HTTP_DEFAILT_PORT  80

#define HOST_IPV4_TYPE    1
#define HOST_DOMAIN_TYPE  2

#define HTTP_PROTOCOL     1
#define HTTPS_PROTOCOL    2

#define SOCKET_SEND_BUFFER_MAX  10*1024
#define SOCKET_RECV_BUFFER_MAX  10*1024

typedef struct hostinfor_t {
    union {
        char ip[IPV4_ADRESS_SIZE];
        char domain[DOMAIN_ADRESS_SIZE];
    } adress;
    char adress_type;
    char protocol;
    int  port;
} hostinfor_t;

typedef struct httpclient {
    int sockfd;
    hostinfor_t host;
} httpclient_t;

int httpclient_init(httpclient_t* httpclient, hostinfor_t host);

int httpclient_deinit(httpclient_t* httpclient);

int httpclient_send_post_msg(httpclient_t* httpclient, char* msg);

int httpclient_wait_respond(httpclient_t* httpclient);
#endif // HTTPCLIENT_H
