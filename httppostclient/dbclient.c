#include "dbclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dbclient_init(dbclient* client, db_type_t type, const char* dbpath) {
    if (client == NULL || dbpath == NULL) {
        return -1;
    }

    client->type = type;

    if (type == TEXT_DB) {
        if (textdb_init(&client->db.textdb, dbpath) != 0) {
            return -1;
        }
    } else {
        client->type = INVALID_DB;
        return -1;
    }

    return 0;
}

int dbclient_deinit(dbclient* client) {
    if (client == NULL) {
        return -1;
    }

    if (client->type == TEXT_DB) {
        if (textdb_deinit(&client->db.textdb) != 0) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

int dbclient_gethost(dbclient* client, hostinfor_t* host) {
    if (client == NULL || host == NULL) {
        return -1;
    }

    switch (client->type) {
        case TEXT_DB:
            if (textdb_gethost(&client->db.textdb, host) != 0) {
                return -1;
            }
            break;
        default:
            return -1;
    }

    return 0;
}
