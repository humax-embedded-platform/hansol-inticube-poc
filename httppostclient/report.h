#ifndef REPORT_H
#define REPORT_H

#include "pthread.h"
#include "linkedlist.h"
#include "httpclient.h"

typedef struct rsp_code_t
{
    int error_code;
    int count;
}rsp_code_t;

typedef struct req_error_t
{
    hostinfor_t host;
    int failure_count;
}req_error_t;


typedef struct report_t
{
    linklist_t error_list;
    linklist_t rsp_list;
} report_t;

int report_init(void);
void report_deinit(void);
void report_add_resp_code(int error_code);
void report_add_req_failure(hostinfor_t host ,int failure_count);
void report_print_result(void);

#endif