#include "worker.h"
#include "buffer.h"
#include <pthread.h>

#define LOG_DBG(format, ...) userdbg_write((format), ##__VA_ARGS__)

typedef struct userdbg_t
{
    worker_t dbg_worker;
    buffer_t dbg_buffer;
    pthread_mutex_t m;
    pthread_cond_t cond;
    int is_completed;
} userdbg_t;

void userdbg_init();
void userdbg_deinit();
void userdbg_write(const char* format, ...);
