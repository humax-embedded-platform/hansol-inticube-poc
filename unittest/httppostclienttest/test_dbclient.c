#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "dbclient.h"
#include "textdb.h"

static void test_dbclient_init(void **state) {
    (void) state;

    dbclient client;

    // test case client and path is null
    assert_int_equal(dbclient_init(NULL, TEXT_DB, NULL), -1);
    assert_int_equal(dbclient_init(NULL, INVALID_DB, NULL), -1);
    
    // test case client or path is null
    client.db.textdb.data = "test_data";
    assert_int_equal(dbclient_init(NULL, TEXT_DB, "../../../example/msg.txt"), -1);
    assert_int_equal(dbclient_init(NULL, INVALID_DB, "../../../example/msg.txt"), -1);
    assert_int_equal(dbclient_init(&client, TEXT_DB, NULL), -1);
    assert_int_equal(dbclient_init(&client, INVALID_DB, NULL), -1);

    // test case client and path is valid
    // TODO: should return 0 if client and path is available
    // assert_int_equal(dbclient_init(&client, TEXT_DB, "../../../example/msg.txt"), 0);
    // assert_int_equal(dbclient_init(&client, INVALID_DB, "../../../example/msg.txt"), 0);
}

static void test_dbclient_deinit(void **state) {
    (void) state;

    dbclient client;

    // test case client is null 
    assert_int_equal(dbclient_deinit(&client), -1);

    // test case client is not null
    // TODO: dbclient_deinit should be return 0 for depend on client.type
    // client.db.textdb.data = "test_data";
    // client.type = TEXT_DB;
    // assert_int_equal(dbclient_deinit(&client), 0); // or -1
    // client.type = INVALID_DB;
    // assert_int_equal(dbclient_deinit(&client), 0); // or -1

}

static void test_dbclient_gethost(void **state) {
    dbclient client;
    hostinfor_t host;

    // test case all null
    assert_int_equal(dbclient_gethost(&client, &host), -1);

    // test case client or host is null
    assert_int_equal(dbclient_gethost(NULL, &host), -1);
    assert_int_equal(dbclient_gethost(&client, NULL), -1);

    // test case client and host is not null
    // TODO: dbclient_gethost should return 0 or -1 depend on client.type
    // client.db.textdb.data="test_data";
    // client.type = TEXT_DB;
    // host.port = 80;
    // assert_int_equal(dbclient_gethost(&client, &host), -1); // or 0
    // client.type = INVALID_DB;
    // assert_int_equal(dbclient_gethost(&client, &host), -1); // or 0
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dbclient_init),
        cmocka_unit_test(test_dbclient_deinit),
        cmocka_unit_test(test_dbclient_gethost),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
