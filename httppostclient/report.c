#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include "report.h"
#include "log.h"
#include "userdbg.h"

static report_t g_report;
static struct timeval first_request_time;
static int failure_count = 0;

static int report_print_time_to_log(void) {
    char buffer[LOG_MESAGE_MAX_SIZE];
    struct timeval last_resp_time;
    gettimeofday(&last_resp_time, NULL);

    struct tm* start = localtime(&first_request_time.tv_sec);
    char start_time_str[80];
    strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%d %H:%M:%S", start);
    snprintf(start_time_str + strlen(start_time_str), sizeof(start_time_str) - strlen(start_time_str), ".%03ld", first_request_time.tv_usec / 1000);

    struct tm* end = localtime(&last_resp_time.tv_sec);
    char end_time_str[80];
    strftime(end_time_str, sizeof(end_time_str), "%Y-%m-%d %H:%M:%S", end);
    snprintf(end_time_str + strlen(end_time_str), sizeof(end_time_str) - strlen(end_time_str), ".%03ld", last_resp_time.tv_usec / 1000);

    int log_len = snprintf(buffer, sizeof(buffer),
        "====================REPORT====================\n"
        "Start time : %s\n"
        "End time   : %s\n"
        "====================REPORT====================\n",
        start_time_str, end_time_str);

    if (log_len < 0 || log_len >= sizeof(buffer)) {
        LOG_DBG("Error creating log message or buffer too small.\n");
        return -1;
    }

    log_write(buffer, log_len);

    return 0;
}

static int report_error_code_cmp(void* curr, void* input_check) {
    rsp_code_t* curr_error = (rsp_code_t*)curr;
    int check_error = *(int*)input_check;

    if (curr_error->error_code == check_error) {
        curr_error->count++;
        return 1;
    }

    return 0;
}

static void report_print_error(void* curr, size_t size) {
    rsp_code_t* error_data = (rsp_code_t*)curr;
    if(error_data->error_code == 28) {
        LOG_INFO("Request Respond: Code %d (Timeout), Count: %d\n", error_data->error_code, error_data->count);
    } else {
        LOG_INFO("Request Respond: Code %d, Count: %d\n", error_data->error_code, error_data->count);        
    }
}

static void report_print_error_request_to_log() {
    char buffer[LOG_MESAGE_MAX_SIZE];
    int write_count = 0;
    int max_log_write_count = 500;

    while (1)
    {
        node_t* node= linklist_remove_from_tail(&g_report.error_list);
        if(node != NULL) {
            req_error_t* req = (req_error_t*)node->data;
            failure_count += req->failure_count;
            for(int i = 0; i < req->failure_count; i++) {
                int log_len = snprintf(buffer, sizeof(buffer), "Request Error: %s\n", req->host.adress.domain);
                if(log_len >= LOG_MESAGE_MAX_SIZE) {
                    log_len = LOG_MESAGE_MAX_SIZE -1;
                }
                buffer[log_len] = '\0';
                log_write(buffer,log_len);

                write_count++;
                if(write_count >= max_log_write_count) {
                    write_count = 0;
                    usleep(2000);
                }
            }
            linklist_node_deinit(node);
        }else {
            break;
        }
    }
}

int report_init() {

    gettimeofday(&first_request_time, NULL);
    linklist_init(&g_report.rsp_list);
    linklist_init(&g_report.error_list);

    return 0;
}

void report_deinit() {
    linklist_deinit(&g_report.rsp_list);
    linklist_deinit(&g_report.error_list);
}

void report_add_resp_code(int error_code) {
    int error = error_code;
    if(linklist_find_and_update(&g_report.rsp_list, report_error_code_cmp, (void*)&error) == 0) {
        rsp_code_t e;
        e.error_code = error_code;
        e.count = 1;
        node_t* node = (node_t*)malloc(sizeof(node_t));
        linklist_node_init(node, (void*)&e, sizeof(e));
        linklist_add(&g_report.rsp_list, node);
    }
}

void report_add_req_failure(hostinfor_t host ,int failure_count) {
    
    req_error_t req;
    req.host = host;
    req.failure_count = failure_count;
    node_t* node = (node_t*)malloc(sizeof(node_t));
    linklist_node_init(node, (void*)&req, sizeof(req_error_t));
    linklist_add(&g_report.error_list, node);
}

void report_print_result() {
    LOG_INFO("Create final report...\n");
    report_print_error_request_to_log();
    report_print_time_to_log();

    node_t* current = (node_t*)&g_report.rsp_list.head;
    LOG_INFO("HTTP Report Results:\n");
    LOG_INFO("====================\n");
    if(failure_count > 0) {
        LOG_INFO("Request Error: Count : %d\n", failure_count);
    }
    linklist_for_each(&g_report.rsp_list, report_print_error);
}
