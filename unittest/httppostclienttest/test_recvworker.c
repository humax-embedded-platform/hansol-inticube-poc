#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "sendworker.h"

#define MSG_FILE_PATH "example/msg.txt"

static recvworker_t rw;
usermsg_t msg;
dbclient db;

void test_recvworker_init(void **state) {
    
    int ret = 0;
    int count = 0;

    ret = recvworker_init(NULL);
    assert_int_equal(ret, -1);

    ret = recvworker_init(&rw);
    assert_int_equal(ret, 0);
    assert_int_not_equal(rw.epoll_fd, -1);
    assert_non_null(rw.wait_resp_list);
    assert_true(rw.recv_thread.active);

}

void test_recvworker_deinit(void **state) {
    int count = 0;
    recvworker_deinit(&rw);
    assert_false(rw.recv_thread.active);
}

void test_recvworker_add_to_waitlist(void **state) {
    int ret = -1;
    httpclient_t client;

}


// Main function to run all tests
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_recvworker_init),
        cmocka_unit_test(test_recvworker_add_to_waitlist),
        cmocka_unit_test(test_recvworker_deinit),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
