#include "recvworker.h"
#include "log.h"
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


static int recvworker_httprespond_timeout_handler(http_resp_t* rsp, char* msg, int len) {
    (void)len;
    (void)msg;

    char buffer[LOG_MESAGE_MAX_SIZE];

    if (rsp != NULL) {
        // Calculate receive time as current time minus 10 seconds
        time_t recv_time = time(NULL) - 10;
        struct tm* recv_tm = localtime(&recv_time);

        char recv_time_str[64];
        strftime(recv_time_str, sizeof(recv_time_str), "%Y-%m-%d %H:%M:%S", recv_tm);

        // Convert send_time to a string
        struct tm* send_tm = localtime(&rsp->send_time);
        char send_time_str[64];
        strftime(send_time_str, sizeof(send_time_str), "%Y-%m-%d %H:%M:%S", send_tm);

        // Check if sendmsg is not NULL and print its content
        const char* sendmsg_content = rsp->sendmsg ? rsp->sendmsg->msg : "No message content";

        // Format the message into the provided buffer
        int len = snprintf(buffer, sizeof(buffer),
                 "Send Time: %s, Receive Time (Timeout - 10s): %s, Host Info: IP = %s, Port = %d, Sent Message: %s, Message: NULL (Timeout)\n",
                 send_time_str, recv_time_str, rsp->client.host.ip, rsp->client.host.port, sendmsg_content);

        log_write(buffer, len);

    } else {
        printf("Received null response pointer, Message: NULL\n");
    }

    return 0;
}


static int recvworker_httprespond_event_handler(http_resp_t* rsp, char* msg, int len) {
    char buffer[LOG_MESAGE_MAX_SIZE];

    if (rsp != NULL) {
        time_t recv_time = time(NULL);
        struct tm* recv_tm = localtime(&recv_time);

        char recv_time_str[64];
        strftime(recv_time_str, sizeof(recv_time_str), "%Y-%m-%d %H:%M:%S", recv_tm);

        struct tm* send_tm = localtime(&rsp->send_time);
        char send_time_str[64];
        strftime(send_time_str, sizeof(send_time_str), "%Y-%m-%d %H:%M:%S", send_tm);

        const char* sendmsg_content = rsp->sendmsg ? rsp->sendmsg->msg : "No message content";

        int size = snprintf(buffer, sizeof(buffer),
                 "Send Time: %s, Receive Time: %s, Host Info: IP = %s, Port = %d, Sent Message: %s, Received Message: %.*s",
                 send_time_str, recv_time_str, rsp->client.host.ip, rsp->client.host.port,
                 sendmsg_content, len, msg);

        log_write(buffer, size);

    } else {
        printf("Received null response pointer, Message: NULL\n");
    }

    return 0;
}

static int recvworker_check_client_timeout_cmp(void *current, void* input) {

    if(current == NULL || input == NULL) {
        return 0;
    }

    int* timeout = (int*)input;
    http_resp_t* resp = (http_resp_t*)current;

    time_t current_time = time(NULL);
    if (difftime(current_time, resp->send_time) >= *timeout) {
        return 1;
    }

    return 0;
}

static int recvworker_check_client_waiting_cmp(void *current, void* input) {

    if(current == NULL) {
        return 0;
    }

    http_resp_t* waitclient = (http_resp_t*)current;
    httpclient_t* checkclient = (httpclient_t*)input;

    if(waitclient->client.sockfd == checkclient->sockfd) {
        return 1;
    }

    return 0;
}

static void recvworker_remove_from_waitlist(recvworker_t* rw, httpclient_t client) {
    if (epoll_ctl(rw->epoll_fd, EPOLL_CTL_DEL, client.sockfd, NULL) == -1) {
        perror("epoll_ctl: EPOLL_CTL_DEL");
    }

    httpclient_deinit(&client);
    linklist_remove(rw->wait_resp_list, recvworker_check_client_waiting_cmp, (void*)&client, 0);
}

static void recvworker_wait_timeout_func(void* arg) {
    recvworker_t* rw = (recvworker_t*)arg;
    int is_sendcompleted = -1;
    int is_waitcompleted = -1;
    int timeout =  MAX_RESPOND_PER_TIME;
    while (1)
    {
        usleep(1000*1000);

        is_sendcompleted = atomic_load(&rw->is_completed);
        is_waitcompleted  = linklist_isempty(rw->wait_resp_list);

        if (is_sendcompleted == 1 && is_waitcompleted == 1) {
            break;
        } 

        node_t* node = NULL;
        while (1)
        {
            node = linklist_find_and_clone(rw->wait_resp_list, recvworker_check_client_timeout_cmp, &timeout, 0);
            if (node != NULL) {
                http_resp_t* rsp = (http_resp_t*)node->data;
                recvworker_httprespond_timeout_handler(rsp,NULL, 0);
                recvworker_remove_from_waitlist(rw, rsp->client);
                link_list_node_deinit(node);
                usleep(1000);
            } else {
                break;
            }
        }
    }
}

static void recvworker_wait_respond_func(void* arg) {
    recvworker_t* rw = (recvworker_t*)arg;
    struct epoll_event events[MAX_RESPOND_PER_TIME];
    httpclient_t client;
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    char buffer[1024];
    node_t* node = NULL;
    int is_sendcompleted = -1;
    int is_waitcompleted = -1;

    while (1) {
        is_sendcompleted = atomic_load(&rw->is_completed);
        is_waitcompleted  = linklist_isempty(rw->wait_resp_list);

        if (is_sendcompleted == 1 && is_waitcompleted == 1) {
            break;
        }

        int nfds = epoll_wait(rw->epoll_fd, events, MAX_RESPOND_PER_TIME, 100);
        if (nfds < 0) {
            printf("epoll_wait error %d\n", nfds);
            break;
        }

        if(nfds == 0) {
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].events & EPOLLIN) {
                client.sockfd = events[i].data.fd;
        
                if (getpeername(client.sockfd, (struct sockaddr*)&peer_addr, &peer_addr_len) == 0) {
                    inet_ntop(AF_INET, &peer_addr.sin_addr, client.host.ip, sizeof(client.host.ip));
                } else {
                    printf("getpeername failed");
                }

                ssize_t bytes_read = recv(client.sockfd, buffer, sizeof(buffer) - 1, 0);

                if (bytes_read < 0) {
                    continue;
                } else if (bytes_read == 0) {
                    continue;
                } else {
                    buffer[bytes_read] = '\0';
                }

                node = linklist_find_and_clone(rw->wait_resp_list, recvworker_check_client_waiting_cmp, &client, 0);
                if (node) {
                    http_resp_t* rsp = (http_resp_t*)node->data;
                    recvworker_httprespond_event_handler(rsp, buffer, bytes_read);
                    recvworker_remove_from_waitlist(rw, rsp->client);
                    link_list_node_deinit(node);
                }
            }
        }
    }
}

int recvworker_init(recvworker_t* rw) {
    if (rw == NULL) {
        return -1;
    }

    atomic_init(&rw->is_completed, 0);

    rw->epoll_fd = epoll_create1(0);
    if (rw->epoll_fd == -1) {
        printf("recvworker_init epoll_create1 error");
        return -1;
    }

    rw->wait_resp_list = (linklist_t*)malloc(sizeof(linklist_t));
    linklist_init(rw->wait_resp_list);

    revctask.task_handler = recvworker_wait_respond_func;
    revctask.arg = rw;
    if (worker_init(&rw->recv_thread, &revctask) != 0) {
        close(rw->epoll_fd);
        return -1;
    }

    timeout_task.task_handler = recvworker_wait_timeout_func;
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

int recvworker_add_to_waitlist(recvworker_t* rw, httpclient_t client, usermsg_t* sendmsg) {
    if (rw == NULL || client.sockfd < 0) {
        return -1;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLONESHOT;
    event.data.fd = client.sockfd;

    if (epoll_ctl(rw->epoll_fd, EPOLL_CTL_ADD, client.sockfd, &event) == -1) {
        printf("recvworker_add_to_waitlist error %d", client.sockfd);
        return -1;
    }

    http_resp_t resp;
    resp.send_time = time(NULL);
    resp.client = client;
    resp.sendmsg = sendmsg;

    node_t* node = (node_t*)malloc(sizeof(node_t));
    link_list_node_init(node, (void*)&resp, sizeof(resp));
    linklist_add(rw->wait_resp_list, node);

    return 0;
}

void recvworker_set_completed(recvworker_t* rw) {
    if (rw != NULL) {
        atomic_store(&rw->is_completed, 1);
    }
}
