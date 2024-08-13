#include "httpclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 

#include "httprequest.h"

int httpclient_init(httpclient_t* httpclient, hostinfor_t host) {
    if (httpclient == NULL) {
        return -1;
    }

    httpclient->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (httpclient->sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);

    if (inet_pton(AF_INET, host.ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(httpclient->sockfd);
        return -1;
    }

    if (connect(httpclient->sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(httpclient->sockfd);
        return -1;
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
    if (httpclient == NULL || msg == NULL) {
        return -1;
    }

    httprequest_t req;

    if (httprequest_init(&req) != 0) {
        return -1;
    }

    httprequest_set_method(&req, "POST");
    httprequest_set_body(&req, msg);

    if (httprequest_build_message(&req) != 0) {
        httprequest_deinit(&req);
        return -1;
    }

    ssize_t sent_bytes = send(httpclient->sockfd, req.message, strlen(req.message), 0);
    if (sent_bytes < 0) {
        perror("Send failed");
        httprequest_deinit(&req);
        return -1;
    }

    httprequest_deinit(&req);

    return 0;
}