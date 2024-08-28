#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include "dbclient.h"
#include "sendworker.h"
#include "message.h"
#include "log.h"
#include "cmd.h"
#include "config.h"
#include "userdbg.h"

int main(int argc, char* argv[]) {
    dbclient db;
    sendworker_t sendworker;
    usermsg_t msg;

    if (cmd_parser(argc, argv) < 0) {
        return -1;
    }

    userdbg_init();

    if(log_init(config_get_log_folder()) <0 ) {
        return -1;
    }

    if (dbclient_init(&db, TEXT_DB, config_get_host_file()) < 0) {
        log_deinit();
        config_deinit();
        LOG_DBG("Can not init database\n");
        return -1;
    }

    if (message_init(&msg, config_get_msg_file()) < 0) {
        dbclient_deinit(&db);
        log_deinit();
        config_deinit();
        LOG_DBG("Can not init message\n");
        return -1;
    }

    if(report_init() < 0) {
        dbclient_deinit(&db);
        log_deinit();
        config_deinit();
        message_deinit(&msg);
    }

    sendworker_set_hostdb(&sendworker, &db);
    sendworker_set_msg(&sendworker, &msg);
    sendworker_set_request_count(&sendworker, config_get_request_count());
    if (sendworker_init(&sendworker) < 0) {
        dbclient_deinit(&db);
        message_deinit(&msg);
        report_deinit();
        log_deinit();
        config_deinit();
        LOG_DBG("Can not init worker\n");
        return -1;
    }

    sendworker_deinit(&sendworker);
    dbclient_deinit(&db);
    message_deinit(&msg);
    report_deinit();
    log_deinit();
    config_deinit();
    userdbg_deinit();

    return 0;
}
