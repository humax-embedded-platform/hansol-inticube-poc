#ifndef REPORT_H
#define REPORT_H

#include "pthread.h"
#include "linkedlist.h"

typedef struct error_code_t
{
    int error_code;
    int count;
}error_code_t;

typedef struct report_t
{
    int failure_count;
    linklist_t rsp_list;
    pthread_mutex_t m;
} report_t;

int report_init(void);
void report_deinit(void);
void report_add_resp_code(int error_code);
void report_add_req_failure(int failure_count);
void report_print_result(void);

#endif