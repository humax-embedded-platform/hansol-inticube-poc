#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "httpclient.h"
#include "httprequest.h"
#include "userdbg.h"

#define HTTP_REQUEST_RETRY_MAX  150

static int httpclient_init_with_ipv4(httpclient_t* httpclient, hostinfor_t host) {
    if (httpclient == NULL) {
        return -1;
    }

    httpclient->host   = host;
    httpclient->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (httpclient->sockfd < 0) {
        LOG_DBG("httpclient_init_with_ipv4: Socket creation failed\n");
        return -1;
    }

    int recv_buffer_size = SOCKET_RECV_BUFFER_MAX;
    int send_buffer_size = SOCKET_SEND_BUFFER_MAX;

    if (setsockopt(httpclient->sockfd, SOL_SOCKET, SO_RCVBUF, &recv_buffer_size, sizeof(recv_buffer_size)) < 0) {
        LOG_DBG("httpclient_init_with_ipv4: Setting SO_RCVBUF failed\n");
        close(httpclient->sockfd);
        return -1;
    }

    if (setsockopt(httpclient->sockfd, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, sizeof(send_buffer_size)) < 0) {
        LOG_DBG("httpclient_init_with_ipv4: Setting SO_SNDBUF failed\n");
        close(httpclient->sockfd);
        return -1;
    }

    int flags = fcntl(httpclient->sockfd, F_GETFL, 0);
    if (flags == -1) {
        LOG_DBG("fcntl(F_GETFL) failed");
        close(httpclient->sockfd);
        return -1;
    }

    if (fcntl(httpclient->sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_DBG("httpclient_init_with_ipv4: fcntl(F_SETFL) failed\n");
        close(httpclient->sockfd);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(host.port);

    if (inet_pton(AF_INET, host.adress.ip, &server_addr.sin_addr) <= 0) {
        LOG_DBG("httpclient_init_with_ipv4: Invalid IP address\n");
        close(httpclient->sockfd);
        return -1;
    }

    int ret = connect(httpclient->sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        if (errno != EINPROGRESS) {
            LOG_DBG("httpclient_init_with_ipv4: Connection failed");
            close(httpclient->sockfd);
            return -1;
        }
    }

    return 0;
}

static int httpclient_init_with_domain(httpclient_t* httpclient, hostinfor_t host) {
    if (httpclient == NULL) {
        return -1;
    }

    httpclient->host = host;

    httpclient->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (httpclient->sockfd < 0) {
        LOG_DBG("httpclient_init_with_domain: Socket creation failed\n");
        return -1;
    }

    int flags = fcntl(httpclient->sockfd, F_GETFL, 0);
    if (flags == -1) {
        LOG_DBG("httpclient_init_with_domain: fcntl(F_GETFL) failed\n");
        close(httpclient->sockfd);
        return -1;
    }

    if (fcntl(httpclient->sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_DBG("httpclient_init_with_domain: fcntl(F_SETFL) failed\n");
        close(httpclient->sockfd);
        return -1;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host.adress.domain, NULL, &hints, &res);
    if (err != 0) {
        close(httpclient->sockfd);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(host.port);
    memcpy(&server_addr.sin_addr, &((struct sockaddr_in*)res->ai_addr)->sin_addr, sizeof(struct in_addr));

    freeaddrinfo(res); 

    int ret = connect(httpclient->sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        if (errno != EINPROGRESS) {
            LOG_DBG("httpclient_init_with_domain: Connection failed\n");
            close(httpclient->sockfd);
            return -1;
        }
    }

    return 0;
}

int httpclient_init(httpclient_t* httpclient, hostinfor_t host) {
    if (httpclient == NULL) {
        return -1;
    }

    if(host.adress_type == HOST_IPV4_TYPE) {
        return httpclient_init_with_ipv4(httpclient, host);
    }

    return httpclient_init_with_domain(httpclient, host);
}

int httpclient_deinit(httpclient_t* httpclient) {
    if (httpclient == NULL) {
        return -1;
    }

    if (close(httpclient->sockfd) < 0) {
        LOG_DBG("httpclient_deinit: Socket close failed\n");
        return -1;
    }

    return 0;
}

int httpclient_send_post_msg(httpclient_t* httpclient, char* msg) {
    httprequest_t req;

    if (httpclient == NULL || msg == NULL) {
        return -1;
    }

    if (httprequest_init(&req) != 0) {
        return -1;
    }

    if( httprequest_set_body(&req, msg) !=0 ) {
        httprequest_deinit(&req);
        return -1;
    }

    if(httpclient->host.adress_type == HOST_IPV4_TYPE) {
        httprequest_set_host(&req, httpclient->host.adress.ip);
    } else {
        httprequest_set_host(&req, httpclient->host.adress.domain);
    }

    if (httprequest_build_message(&req) != 0) {
        httprequest_deinit(&req);
        return -1;
    }

    size_t msg_length = strlen(req.message);
    size_t total_send = 0;
    int    retry_count = 0;
    while (total_send < msg_length) {
        ssize_t sent = send(httpclient->sockfd, req.message + total_send, msg_length - total_send, 0);
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                retry_count++;
                if(retry_count > HTTP_REQUEST_RETRY_MAX) {
                    httprequest_deinit(&req);
                    return -1;                    
                }
                usleep(2000);
                continue;
            } else {
                httprequest_deinit(&req);
                return -1;
            }
        }

        total_send += sent;
    }

    httprequest_deinit(&req);

    return 0;
}