#include "httpclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <fcntl.h>
#include <errno.h>
#include "httprequest.h"

int httpclient_init(httpclient_t* httpclient, hostinfor_t host) {
    if (httpclient == NULL) {
        return -1;
    }

    httpclient->host   = host;
    httpclient->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (httpclient->sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Set the socket to non-blocking mode
    int flags = fcntl(httpclient->sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL) failed");
        close(httpclient->sockfd);
        return -1;
    }

    if (fcntl(httpclient->sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL) failed");
        close(httpclient->sockfd);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(host.port);

    if (inet_pton(AF_INET, host.ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(httpclient->sockfd);
        return -1;
    }

    int ret = connect(httpclient->sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        if (errno != EINPROGRESS) {
            perror("Connection failed");
            close(httpclient->sockfd);
            return -1;
        }
    }

    return 0;
}
int httpclient_deinit(httpclient_t* httpclient) {
    if (httpclient == NULL) {
        return -1;
    }

    if (close(httpclient->sockfd) < 0) {
        perror("Socket close failed");
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

    httprequest_set_body(&req, msg);
    httprequest_set_host(&req, httpclient->host.ip);

    if (httprequest_build_message(&req) != 0) {
        httprequest_deinit(&req);
        return -1;
    }

    while (send(httpclient->sockfd, req.message, strlen(req.message), 0) <= 0) {
        usleep(100);
    }

    httprequest_deinit(&req);

    return 0;
}