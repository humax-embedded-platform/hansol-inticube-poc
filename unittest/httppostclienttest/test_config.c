#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define TEST_FILE "test_file.txt"
#define LARGE_FILE "large_file.txt"

// Define the config structure and max file size for testing
#define MAX_INPUT_FILE_SIZE 2048

const char *file_path = "../testdata/msg.txt";

struct {
    char* msg_file;
} config;

// Test for config_init function
static void test_config_init(void **state) {
    (void) state; //unused

    config_init();
    assert_int_equal(config_get_request_count(), 0);
}

// Test for config_deinit function
static void test_config_deinit(void **state) {
    (void) state; //unused

    config_init();

    assert_int_equal(config_set_msg_file("../../../example/msg.txt"), 0);
    assert_int_equal(config_set_host_file("../../../example/msg.txt"), 0);
    assert_int_equal(config_set_log_folder("../../../example/log_folder/"), 0);
    
    config_deinit();

    assert_null(config_get_msg_file());
    assert_null(config_get_host_file());
    assert_null(config_get_log_folder());
    assert_int_equal(config_get_request_count(), 0);
}

// Test for config_set_msg_file function 
static void test_config_set_msg_file(void **state) {
    (void) state;

    // test valid file
    assert_int_equal(config_set_msg_file("../../../example/msg.txt"), 0);
    // test invalid
    assert_int_equal(config_set_msg_file("../../../example/invalid_file.txt"), -1);
    // note type txt
    assert_int_equal(config_set_msg_file("../../../example/msg.md"), -1);
    // can not get file size or file too large
    assert_int_equal(config_set_msg_file("../../../example/3kb_size.md"), -1);

    assert_non_null(config_get_msg_file());
}

// Test for config_set_host_file function
static void test_config_set_host_file(void **state) {
    (void) state;

    // test valid file
    assert_int_equal(config_set_host_file("../../../example/msg.txt"), 0);
    // test invalid file
    assert_int_equal(config_set_host_file("../../../example/invalid_file.txt"), -1);

    assert_non_null(config_get_host_file());
}

// Test for config_set_log_folder function
static void test_config_set_log_folder(void **state) {
    (void) state;

    // test valid folder
    assert_int_equal(config_set_log_folder("../../../example/log_folder/"), 0);
    // test invalid folder
    // TODO: if folder is invalid, create folder and return 0
    // assert_int_equal(config_set_log_folder("../../../example/invalid_folder/"), -1);

    assert_non_null(config_get_log_folder());
}

// Test for config_set_request_count function
static void test_config_set_request_count(void **state) {
    // test request count < 0
    assert_int_equal(config_set_request_count(-1), -1);

    // test request count = 0
    assert_int_equal(config_set_request_count(0), -1);

    // test request count = 1
    assert_int_equal(config_set_request_count(1), 0);
    assert_int_equal(config_get_request_count(), 1);


}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_config_init),
        cmocka_unit_test(test_config_deinit),
        cmocka_unit_test(test_config_set_msg_file),
        cmocka_unit_test(test_config_set_host_file),
        cmocka_unit_test(test_config_set_log_folder),
        cmocka_unit_test(test_config_set_request_count),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
