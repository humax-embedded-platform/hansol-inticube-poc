#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include <stdbool.h>

typedef struct task {
    void (*task_handler)(void* arg);
    void* arg;
} task_t;

typedef struct worker {
    pthread_t thread;
    task_t* task;
    bool active;
} worker_t;

int worker_init(worker_t* w, task_t* task);
void worker_deinit(worker_t* w);

#endif // WORKER_H
