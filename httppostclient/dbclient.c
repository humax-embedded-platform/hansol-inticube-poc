#include "dbclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

int dbclient_init(dbclient* client, db_type_t type, const char* dbpath) {
    if (client == NULL || dbpath == NULL) {
        fprintf(stderr, "dbclient_init: Invalid arguments\n");
        return -1;
    }

    client->type = type;
    client->current_index = 0;
    atomic_flag_clear(&client->index_lock);

    if (type == TEXT_DB) {
        if (textdb_init(&client->db.textdb, dbpath) != 0) {
            fprintf(stderr, "dbclient_init: Failed to initialize textdb\n");
            return -1;
        }
        printf("dbclient_init: Successfully initialized textdb with path %s\n", dbpath);
    } else {
        client->type = INVALID_DB;
        fprintf(stderr, "dbclient_init: Unsupported database type %d\n", type);
        return -1;
    }

    return 0;
}

int dbclient_deinit(dbclient* client) {
    if (client == NULL) {
        fprintf(stderr, "dbclient_deinit: Invalid argument\n");
        return -1;
    }

    if (client->type == TEXT_DB) {
        if (textdb_deinit(&client->db.textdb) != 0) {
            fprintf(stderr, "dbclient_deinit: Failed to deinitialize textdb\n");
            return -1;
        }
        printf("dbclient_deinit: Successfully deinitialized textdb\n");
    } else {
        fprintf(stderr, "dbclient_deinit: Unsupported database type %d\n", client->type);
        return -1;
    }

    return 0;
}

int dbclient_gethost(dbclient* client, hostinfor_t* host) {
    if (client == NULL || host == NULL) {
        fprintf(stderr, "dbclient_gethost: Invalid arguments\n");
        return -1;
    }

    // Acquire spinlock
    while (atomic_flag_test_and_set(&client->index_lock)) {
        // Spin until the lock is acquired
    }

    int current_index = client->current_index;
    client->current_index += 1;

    // Release spinlock
    atomic_flag_clear(&client->index_lock);

    switch (client->type) {
        case TEXT_DB:
            if (textdb_gethost(&client->db.textdb, host, current_index) != 0) {
                return -1;
            }
            break;
        default:
            return -1;
    }

    return 0;
}
