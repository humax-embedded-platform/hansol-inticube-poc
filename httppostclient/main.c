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
#include "clientmanager.h"

int main(int argc, char* argv[]) {
    dbclient db;
    sendworker_t sendworker;
    usermsg_t msg;
    clientmanager_t clientmanager;

    if (cmd_parser(argc, argv) < 0) {
        return -1;
    }

    if(userdbg_init() < 0) {
        return;
    }

    if(log_init(config_get_log_folder()) <0 ) {
        userdbg_deinit();
        return -1;
    }

    if (dbclient_init(&db, TEXT_DB, config_get_host_file()) < 0) {
        LOG_DBG("Can not init database\n");
        log_deinit();
        config_deinit();
        userdbg_deinit();
        return -1;
    }

    if (message_init(&msg, config_get_msg_file()) < 0) {
        LOG_DBG("Can not init message\n");
        dbclient_deinit(&db);
        log_deinit();
        config_deinit();
        userdbg_deinit();
        return -1;
    }

    if(report_init() < 0) {
        LOG_DBG("Can not init report\n");
        dbclient_deinit(&db);
        log_deinit();
        config_deinit();
        message_deinit(&msg);
        userdbg_deinit();
    }

    clientmanager_set_hostdb(&clientmanager,&db);
    clientmanager_set_request_count(&clientmanager,config_get_request_count());
    if(clientmanager_init(&clientmanager) < 0) {
        LOG_DBG("Can not init clientmanager\n");
        dbclient_deinit(&db);
        log_deinit();
        config_deinit();
        message_deinit(&msg);
        userdbg_deinit();
    }

    sendworker_set_msg(&sendworker, &msg);
    sendworker_set_clientmanager(&sendworker,&clientmanager);
    if (sendworker_init(&sendworker) < 0) {
        LOG_DBG("Can not init worker\n");
        clientmanager_deinit(&clientmanager);
        dbclient_deinit(&db);
        message_deinit(&msg);
        report_deinit();
        log_deinit();
        config_deinit();
        userdbg_deinit();
        return -1;
    }

    sendworker_deinit(&sendworker);
    clientmanager_deinit(&clientmanager);
    
    dbclient_deinit(&db);
    message_deinit(&msg);
    report_deinit();
    log_deinit();
    config_deinit();
    userdbg_deinit();



    return 0;
}
