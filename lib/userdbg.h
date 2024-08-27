#include "worker.h"
#include "buffer.h"
#include <pthread.h>

typedef struct userdbg_t
{
    worker_t dbg_worker;
    buffer_t dbg_buffer;
    pthread_mutex_t m;
    int is_completed;
} userdbg_t;

void userdbg_init();
void userdbg_deinit();
void userdbg_write(const char* format, ...);
