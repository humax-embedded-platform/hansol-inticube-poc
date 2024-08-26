#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "sendworker.h"

#define MSG_FILE_PATH "example/msg.txt"

static sendworker_t sw;
usermsg_t msg;
dbclient db;

void test_sendworker_init(void **state) {
    
    int ret = 0;
    int count = 0;

    ret = sendworker_init(NULL);
    assert_int_equal(ret, -1);

    ret = sendworker_init(&sw);
    assert_int_equal(ret, -1);
    
    sendworker_set_hostdb(&sw, &db);
    sendworker_set_msg(&sw, &db);
    ret = sendworker_init(&sw);
    assert_int_equal(ret, 0);
    for (count = 0; count < MAX_SEND_WORKER; count++)
    {
        assert_true(sw.workers[count].active);
    }
}

void test_sendworker_deinit(void **state) {
    int count = 0;
    sendworker_deinit(&sw);

    for (count = 0; count < MAX_SEND_WORKER; count++)
    {
        assert_false(sw.workers[count].active);
    }
}

void test_sendworker_set_hostdb(void **state) {
    sw.hostdb = NULL;
    sendworker_set_hostdb(&sw, &db);
    assert_non_null(sw.hostdb);
}

void test_sendworker_set_msg(void **state) {
    sw.msg = NULL;
    sendworker_set_msg(&sw, &msg);
    assert_non_null(sw.msg);
}

void test_sendworker_set_request_count(void **state) {
    int count = 100;
    sendworker_set_request_count(&sw, count);
    assert_int_equal(sw.request_count, count);
}

// Main function to run all tests
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sendworker_init),
        cmocka_unit_test(test_sendworker_deinit),
        cmocka_unit_test(test_sendworker_set_hostdb),
        cmocka_unit_test(test_sendworker_set_request_count),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
