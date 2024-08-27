
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "userdbg.h"

#define USERDBG_BUFFER_CAPACITY 1000
#define USERDBG_BUFFER_SIZE     1024
// Static declaration of userdbg_t variable
static userdbg_t g_userdbg; 
static task_t    g_dbgtask;


static void userdbg_set_completed(int status) {
    pthread_mutex_lock(&g_userdbg.m);
    g_userdbg.is_completed = status;
    pthread_mutex_unlock(&g_userdbg.m);
}

static int userdbg_get_completed() {
    int status;
    pthread_mutex_lock(&g_userdbg.m);
    status = g_userdbg.is_completed;
    pthread_mutex_unlock(&g_userdbg.m);

    return status;
}

static void userdbg_task_handler(void* arg) {
    log_entry_t entry;
    char dbg_buff[USERDBG_BUFFER_SIZE];

    while (1)
    {
        if(buffer_read(&g_userdbg.dbg_buffer, &entry) == 0) {
            memcpy(dbg_buff,entry.data,entry.size);
            dbg_buff[entry.size] = '\0';
            printf("%s", dbg_buff);
            buffer_log_entry_deinit(&entry);
        } else {
            if(userdbg_get_completed() == 1) {
                while (buffer_read(&g_userdbg.dbg_buffer, &entry) == 0) {
                    printf("%s", entry.data);
                    buffer_log_entry_deinit(&entry);
                }
                break;
            } else{
                usleep(50*1000);
            }
        }
    }
    
}

void userdbg_init() {
    buffer_init(&g_userdbg.dbg_buffer,USERDBG_BUFFER_CAPACITY);

    if (pthread_mutex_init(&g_userdbg.m, NULL) != 0) {
        perror("Failed to initialize mutex");
        buffer_deinit(&g_userdbg.dbg_buffer);
        return;
    }

    userdbg_set_completed(0);

    g_dbgtask.task_handler = userdbg_task_handler;
    g_dbgtask.arg = (void*)&g_userdbg;
    if (worker_init(&g_userdbg.dbg_worker, &g_dbgtask) != 0) {
        perror("Failed to initialize worker");
        buffer_deinit(&g_userdbg.dbg_buffer);
        pthread_mutex_destroy(&g_userdbg.m);
        return;
    }
}

void userdbg_deinit() {
    userdbg_set_completed(1);

    worker_deinit(&g_userdbg.dbg_worker);
    buffer_deinit(&g_userdbg.dbg_buffer);
    pthread_mutex_destroy(&g_userdbg.m);
}

void userdbg_write(const char* format, ...) {
    if (format == NULL) {
        return;
    }

    if (userdbg_get_completed() == 1) {
        return;
    }

    char buffer[USERDBG_BUFFER_SIZE];
    va_list args;

    va_start(args, format);

    int length = vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    if (length < 0) {
        perror("Formatting error");
        return;
    }

    buffer_write(&g_userdbg.dbg_buffer, buffer, (size_t)length);
}