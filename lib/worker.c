#include "worker.h"
#include <stdlib.h>
#include <stdio.h>

static void* worker_thread_func(void* arg) {
    worker_t* w = (worker_t*)arg;

    if (w && w->task->task_handler) {
        w->task->task_handler(w->task->arg);
    }

    return NULL;
}

int worker_init(worker_t* w, task_t* task) {
    if (w == NULL || task->task_handler == NULL) {
        return -1;
    }

    w->task = task;
    w->active = true;

    if (pthread_create(&w->thread, NULL, worker_thread_func, w) != 0) {
        perror("Failed to create thread");
        return -1;
    }

    return 0;
}

void worker_deinit(worker_t* w) {
    if (w == NULL) {
        return;
    }

    if (w->active) {
        pthread_join(w->thread, NULL);
        w->active = false;
    }
}
