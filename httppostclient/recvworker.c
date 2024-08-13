#include "recvworker.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include <stdatomic.h>
#include <arpa/inet.h>

static task_t revctask;
static task_t timeout_task;

static int recvworker_check_client_timeout(void *current, void* cmp) {

    if(current == NULL || cmp == NULL) {
        return 0;
    }

    int* timeout = (int*)cmp;
    http_resp_t* resp = (http_resp_t*)current;

    time_t current_time = time(NULL);
    if (difftime(current_time, resp->start_time) >= *timeout) {
        return 1;
    }

    return 0;
}

static int recvworker_client_timeout_cb(void *client) {
    
    recvworker_remove_from_epoll();
}

static int recvworker_check_client_exit(void *current, void* cmp) {

    if(current == NULL) {
        return 0;
    }

    http_resp_t* waitclient = (http_resp_t*)current;
    httpclient_t* checkclient = (httpclient_t*)cmp;

    if(waitclient->client.sockfd == checkclient->sockfd) {
        return 1;
    }

    return 0;
}

static int recvworker_client_responded_cb(void *client) {

}

static int recvworker_httprespond_event_handler(hostinfor_t host, char* msg, int len) {
}

static void* recvworker_httprespond_timeout_handler(void* arg) {
    recvworker_t* rw = (recvworker_t*)arg;

    while (1)
    {
        if(atomic_load(&rw->is_completed)) {
            if(rw->wait_resp_list->size == 0) {
                break;
            }
        }

        int timeout = RECV_RESPOND_TIMEOUT;

        linklist_remove_with_condition(rw->wait_resp_list, &timeout, recvworker_check_client_timeout, recvworker_client_timeout_cb);

        usleep(1000*1000);
    }
}

static void* recvworker_thread_func(void* arg) {
    recvworker_t* rw = (recvworker_t*)arg;
    struct epoll_event events[MAX_RESPOND_PER_TIME];
    httpclient_t client;
    char buffer[1024];

    while (1) {
        int nfds = epoll_wait(rw->epoll_fd, events, MAX_RESPOND_PER_TIME, -1);
        if (nfds < 0) {
            printf("epoll_wait error %d\n", nfds);
            break;
        }

        if (nfds == 0) {
            if (atomic_load(&rw->is_completed)) {
                break;
            }

            usleep(100 * 1000);
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].events & EPOLLIN) {
                int sockfd = events[i].data.fd;
                struct sockaddr_in peer_addr;
                socklen_t peer_addr_len = sizeof(peer_addr);
                hostinfor_t host;
        
                if (getpeername(sockfd, (struct sockaddr*)&peer_addr, &peer_addr_len) == 0) {
                    inet_ntop(AF_INET, &peer_addr.sin_addr, host.ip, sizeof(host.ip));
                    printf("Peer IP address: %s, Port: %d\n", host.ip, ntohs(peer_addr.sin_port));
                } else {
                    printf("getpeername failed");
                }

                ssize_t bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
                if (bytes_read < 0) {
                    perror("recv failed");
                } else if (bytes_read == 0) {
                    printf("Connection closed by peer on socket %d\n", sockfd);
                    client.sockfd = sockfd;
                    recvworker_remove_from_epoll(rw, client);
                    close(sockfd);
                } else {
                    buffer[bytes_read] = '\0';
                    client.sockfd = sockfd;
                    recvworker_remove_from_epoll(rw, client);
                    close(sockfd);
                    printf("Received response on socket %d:\n%s\n", sockfd, buffer);
                }

                if(bytes_read >=0) {
                    recvworker_httprespond_event_handler(host, buffer, bytes_read);
                }
            }
        }
    }

    return NULL;
}

int recvworker_init(recvworker_t* rw) {
    if (rw == NULL) {
        return -1;
    }

    atomic_store(&rw->is_completed, false);
    rw->epoll_fd = epoll_create1(0);
    if (rw->epoll_fd == -1) {
        printf("recvworker_init epoll_create1 error");
        return -1;
    }

    rw->wait_resp_list = (linklist_t*)malloc(sizeof(linklist_t));
    linklist_init(rw->wait_resp_list);

    revctask.task_handler = recvworker_thread_func;
    revctask.arg = rw;
    if (worker_init(&rw->recv_thread, &revctask) != 0) {
        close(rw->epoll_fd);
        return -1;
    }

    timeout_task.task_handler = recvworker_httprespond_timeout_handler;
    timeout_task.arg = rw;

    if (worker_init(&rw->timeout_thread, &timeout_task) != 0) {
        worker_deinit(&rw->recv_thread);
        close(rw->epoll_fd);
        return -1;
    }

    return 0;
}

void recvworker_deinit(recvworker_t* rw) {
    if (rw == NULL) {
        return;
    }

    worker_deinit(&rw->recv_thread);
    worker_deinit(&rw->timeout_thread);
    linklist_deinit(rw->wait_resp_list);
    free(rw->wait_resp_list);
    close(rw->epoll_fd);
}

int recvworker_add_to_epoll(recvworker_t* rw, httpclient_t client) {
    if (rw == NULL || client.sockfd < 0) {
        return -1;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client.sockfd;

    if (epoll_ctl(rw->epoll_fd, EPOLL_CTL_ADD, client.sockfd, &event) == -1) {
        printf("recvworker_add_to_epoll error %d", client.sockfd);
        return -1;
    }

    http_resp_t resp;
    resp.start_time = time(NULL);
    resp.client = client;

    node_t* node = (node_t*)malloc(sizeof(node_t));
    link_list_node_init(rw->wait_resp_list, (void*)&resp, sizeof(resp));
    linklist_add(rw->wait_resp_list, node);

    return 0;
}

int recvworker_remove_from_epoll(recvworker_t* rw, httpclient_t client) {
    if (rw == NULL || client.sockfd < 0) {
        return -1;
    }

    if (epoll_ctl(rw->epoll_fd, EPOLL_CTL_DEL, client.sockfd, NULL) == -1) {
        printf("recvworker_remove_from_epoll error %d", client.sockfd);
        return -1;
    }

    return 0;
}

void recvworker_set_completed(recvworker_t* rw) {
    if (rw != NULL) {
        atomic_store(&rw->is_completed, true);
    }
}
