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

static int httpclient_init_with_ipv4(httpclient_t* httpclient, hostinfor_t host) {
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

    printf("host.adress.ip %s\n",host.adress.ip);
    printf("host.adress.port %d\n",host.port);

    if (inet_pton(AF_INET, host.adress.ip, &server_addr.sin_addr) <= 0) {
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

static int httpclient_init_with_domain(httpclient_t* httpclient, hostinfor_t host) {
    if (httpclient == NULL) {
        return -1;
    }

    httpclient->host = host;

    // Create the socket
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

    // Resolve the domain name to an IP address
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;         // Only IPv4 addresses
    hints.ai_socktype = SOCK_STREAM;   // TCP stream sockets

    int err = getaddrinfo(host.adress.domain, NULL, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        close(httpclient->sockfd);
        return -1;
    }

    // Set the resolved IP address and port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(host.port);
    memcpy(&server_addr.sin_addr, &((struct sockaddr_in*)res->ai_addr)->sin_addr, sizeof(struct in_addr));

    freeaddrinfo(res); 

    // Attempt to connect
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

    if(httpclient->host.adress_type == HOST_IPV4_TYPE) {
        httprequest_set_host(&req, httpclient->host.adress.ip);
    } else {
        httprequest_set_host(&req, httpclient->host.adress.domain);
    }

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