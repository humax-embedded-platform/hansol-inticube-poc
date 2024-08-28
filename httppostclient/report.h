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

void report_init(void);
void report_deinit(void);
void report_add_result(int error_code);
void report_print_result(void);

#endif