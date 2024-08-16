#include "worker.h"
#include "logwriter.h"

typedef struct logrecv_t
{
    int logrecv_fd;
    worker_t recv_worker;
    logwriter_t writer;
} logrecv_t;


void logrecv_init(logrecv_t* receiver);
void logrecv_deinit(logrecv_t* receiver);