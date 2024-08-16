#include <stdio.h>
#include <stdlib.h>

#include "dbclient.h"
#include "sendworker.h"
#include "message.h"
#include "log.h"
int main() {
    dbclient db;
    sendworker_t sendworker;
    usermsg_t msg;

    log_init();

    if( dbclient_init(&db,TEXT_DB,"../hostdb.txt") < 0) {
        printf("Can not init database\n");
        return -1;
    }

    if (message_init(&msg,"../msg.txt") < 0) {
        printf("Can not init message\n");
        return -1;
    }

    if (sendworker_init(&sendworker, &msg, &db) < 0 ) {
        printf("Can not init worker\n");
        return -1;
    }

    sendworker_deinit(&sendworker);
    dbclient_deinit(&db);
    message_deinit(&msg);

    log_deinit();

    printf("end of program");
}

