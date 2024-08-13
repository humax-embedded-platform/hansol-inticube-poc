#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include "common.h"

typedef struct httpclient {
    int sockfd;
    hostinfor_t host;
} httpclient_t;

int httpclient_init(httpclient_t* httpclient, hostinfor_t host);

int httpclient_deinit(httpclient_t* httpclient);

int httpclient_send_post_msg(httpclient_t* httpclient, char* msg);

int httpclient_wait_respond(httpclient_t* httpclient);
#endif // HTTPCLIENT_H
