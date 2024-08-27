#include <stdio.h>
#include <stdlib.h>
#include "report.h"

static int report_error_code_cmp(void* curr, void* input_check) {
    error_code_t* curr_error = (error_code_t*)curr;
    int check_error = *(int*)input_check;

    if (curr_error->error_code == check_error) {
        curr_error->count++;
        return 1;
    }

    return 0;
}

static void report_print_error(void* curr, size_t size) {
    error_code_t* error_data = (error_code_t*)curr;
    printf("Respond Code: %d, Count: %d\n", error_data->error_code, error_data->count);
}

void report_init(report_t* rp) {
    linklist_init(&rp->error_list);
}

void report_deinit(report_t* rp) {
    linklist_deinit(&rp->error_list);
}

void report_add_result(report_t* rp, int error_code) {
    int error = error_code;
    if(linklist_find_and_update(&rp->error_list, report_error_code_cmp, (void*)&error) == 0) {
        error_code_t e;
        e.error_code = error_code;
        e.count = 1;
        node_t* node = (node_t*)malloc(sizeof(node_t));
        linklist_node_init(node, (void*)&e, sizeof(e));
        linklist_add(&rp->error_list, node);
    }
}

void report_print_result(report_t* rp) {
    node_t* current = &rp->error_list.head;
    printf("HTTP Report Results:\n");
    printf("====================\n");
    linklist_for_each(&rp->error_list, report_print_error);
}