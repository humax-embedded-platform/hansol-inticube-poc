
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "userdbg.h"

#define USERDBG_BUFFER_CAPACITY 1000
#define USERDBG_BUFFER_SIZE     1024

#define USERDBG_STATUS_COMPLETED   1
#define USERDBG_INIT_STATUS_INPROGRESS 0

static userdbg_t g_userdbg; 
static task_t    g_dbgtask;

static int is_buffer_have_data = 0;

static void userdbg_set_completed(int status) {
    pthread_mutex_lock(&g_userdbg.m);
    g_userdbg.is_completed = status;
    pthread_mutex_unlock(&g_userdbg.m);

    pthread_cond_signal(&g_userdbg.cond);
}

static void userdbg_set_buffer_status(int status) {
    pthread_mutex_lock(&g_userdbg.m);
    is_buffer_have_data = status;
    pthread_mutex_unlock(&g_userdbg.m);

    pthread_cond_signal(&g_userdbg.cond);
}

static void userdbg_task_handler(void* arg) {
    log_entry_t entry;

    while (1)
    {
        pthread_mutex_lock(&g_userdbg.m);

        while (g_userdbg.is_completed == USERDBG_INIT_STATUS_INPROGRESS && is_buffer_have_data == 0) {
            pthread_cond_wait(&g_userdbg.cond, &g_userdbg.m);
        }

        if(g_userdbg.is_completed == USERDBG_STATUS_COMPLETED) {
            while (buffer_read(&g_userdbg.dbg_buffer, &entry) == 0) {
                entry.data[entry.size] = '\0';
                printf("%s", entry.data);
                buffer_log_entry_deinit(&entry);
            }

            pthread_mutex_unlock(&g_userdbg.m);    
            break;       
        }

        if(is_buffer_have_data == 1) {
            is_buffer_have_data = 0;
        }
    
        pthread_mutex_unlock(&g_userdbg.m);

        while (buffer_read(&g_userdbg.dbg_buffer, &entry) == 0) {
            if (entry.size >= USERDBG_BUFFER_SIZE) {
                entry.size = USERDBG_BUFFER_SIZE - 1;
            }
            entry.data[entry.size] = '\0';
            printf("%s", entry.data);
            buffer_log_entry_deinit(&entry);
        }
    }
}

void userdbg_init() {
    buffer_init(&g_userdbg.dbg_buffer,USERDBG_BUFFER_CAPACITY);

    if (pthread_mutex_init(&g_userdbg.m, NULL) != 0) {
        buffer_deinit(&g_userdbg.dbg_buffer);
        return;
    }

    if (pthread_cond_init(&g_userdbg.cond, NULL) != 0) {
        pthread_mutex_destroy(&g_userdbg.m);
        buffer_deinit(&g_userdbg.dbg_buffer);
        return;
    }

    userdbg_set_completed(USERDBG_INIT_STATUS_INPROGRESS);

    g_dbgtask.task_handler = userdbg_task_handler;
    g_dbgtask.arg = (void*)&g_userdbg;
    if (worker_init(&g_userdbg.dbg_worker, &g_dbgtask) != 0) {
        buffer_deinit(&g_userdbg.dbg_buffer);
        pthread_cond_destroy(&g_userdbg.cond);
        pthread_mutex_destroy(&g_userdbg.m);
        return;
    }
}

void userdbg_deinit() {
    userdbg_set_completed(USERDBG_STATUS_COMPLETED);
    worker_deinit(&g_userdbg.dbg_worker);
    buffer_deinit(&g_userdbg.dbg_buffer);
    pthread_cond_destroy(&g_userdbg.cond);
    pthread_mutex_destroy(&g_userdbg.m);
}

void userdbg_write(const char* format, ...) {
    if (format == NULL || g_userdbg.is_completed == USERDBG_STATUS_COMPLETED) {
        return;
    }

    char buffer[USERDBG_BUFFER_SIZE];
    va_list args;

    va_start(args, format);

    int length = vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    if (length < 0) {
        return;
    }

    buffer_write(&g_userdbg.dbg_buffer, buffer, (size_t)length);

    userdbg_set_buffer_status(1);
}