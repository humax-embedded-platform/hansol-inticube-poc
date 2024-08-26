#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "cmd.h"
#include "config.h"

// Test cases
static void test_cmd_parser_valid_args(void **state) {
    (void)state; // Unused

    char *argv[] = {"program", "--request", "5", "--input", "input.txt", "--host", "host.txt", "--log", "log_folder"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    int result = cmd_parser(argc, argv);

    assert_int_equal(result, -1);
}

static void test_cmd_parser_missing_request(void **state) {
    (void)state; // Unused

    char *argv[] = {"program", "--input", "input.txt", "--host", "host.txt"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    assert_int_equal(cmd_parser(argc, argv), -1);
}

static void test_cmd_parser_invalid_request(void **state) {
    (void)state; // Unused

    char *argv[] = {"program", "--request", "invalid", "--input", "input.txt", "--host", "host.txt"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    assert_int_equal(cmd_parser(argc, argv), -1);
}

// More test cases...

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_cmd_parser_valid_args),
        cmocka_unit_test(test_cmd_parser_missing_request),
        cmocka_unit_test(test_cmd_parser_invalid_request),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
