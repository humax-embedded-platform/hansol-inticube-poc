#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "message.h"

#define MSG_FILE_PATH "../../../example/msg.txt"

// Mock the textdb functions
void test_message_init(void **state) {
    int ret = 0;
    usermsg_t msg;
    char *file_path = NULL;

    // 
    ret = message_init(NULL, "path");
    assert_int_equal(ret, -1);

    ret = message_init(&msg, NULL);
    assert_int_equal(ret, -1);
    
    ret = message_init(&msg, MSG_FILE_PATH);
    assert_int_equal(ret, 0);
    assert_non_null(msg.msg);
}

void test_message_deinit(void **state) {
     int ret = 0;
    usermsg_t msg;
    

    ret = message_init(&msg, MSG_FILE_PATH);
    assert_int_equal(ret, 0);
    assert_non_null(msg.msg);
    
    message_deinit(&msg);
    assert_null(msg.msg);
}

// Main function to run all tests
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_message_init),
        cmocka_unit_test(test_message_deinit),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
