#include <stdio.h>
#include <stdlib.h>
#include <time.h>  // Include for time functions

#include "dbclient.h"
#include "sendworker.h"
#include "message.h"
#include "log.h"

int main() {
    dbclient db;
    sendworker_t sendworker;
    usermsg_t msg;
    clock_t start_time, end_time;
    double elapsed_time_ms;

    // Init time start
    start_time = clock();

    log_init();

    if (dbclient_init(&db, TEXT_DB, "../hostdb.txt") < 0) {
        printf("Can not init database\n");
        return -1;
    }

    if (message_init(&msg, "../msg.txt") < 0) {
        printf("Can not init message\n");
        return -1;
    }

    sendworker_set_hostdb(&sendworker, &db);
    sendworker_set_msg(&sendworker, &msg);
    sendworker_set_request_count(&sendworker, 1000);

    // Init time end
    end_time = clock();
    elapsed_time_ms = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000;
    printf("Initialization time: %.2f milliseconds\n", elapsed_time_ms);

    // Send time start
    start_time = clock();

    if (sendworker_init(&sendworker) < 0) {
        printf("Can not init worker\n");
        return -1;
    }

    sendworker_deinit(&sendworker);

    // Send time end
    end_time = clock();
    elapsed_time_ms = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000;
    printf("Send time: %.2f milliseconds\n", elapsed_time_ms);

    dbclient_deinit(&db);
    message_deinit(&msg);

    log_deinit();

    printf("End of program\n");

    return 0;
}
