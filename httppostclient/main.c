#include <stdio.h>
#include <stdlib.h>

#include "dbclient.h"
#include "sendworker.h"
#include "message.h"

int main() {
    dbclient db;
    sendworker_t sendworker;
    usermsg_t msg;


    dbclient_init(&db,TEXT_DB,"../test/hostdb.txt");
    message_init(&msg,"../test/msg.txt");
    sendworker_init(&sendworker, &msg, &db);

    sendworker_deinit(&sendworker);
    dbclient_deinit(&db);
    message_deinit(&msg);
    
    printf("end of program");
}
