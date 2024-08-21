#ifndef REPORT_H
#define REPORT_H

#include "linkedlist.h"

typedef struct error_code_t
{
    int error_code;
    int count;
}error_code_t;

typedef struct report_t
{
    linklist_t error_list;
} report_t;

void report_init(report_t* rp);
void report_deinit(report_t* rp);
void report_add_result(report_t* rp, int error_code);
void report_print_result(report_t* rp);

#endif