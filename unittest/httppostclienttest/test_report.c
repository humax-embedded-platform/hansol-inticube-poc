#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "report.h"


void test_report_init(void **state) {
    int ret = -1;
    ret = report_init();
    assert_int_equal(ret, 0);
}

void test_report_add_resp_code(void **state) {
    report_add_resp_code(10);
}
void test_report_add_req_failure(void **state) {
    report_add_req_failure(20);
}
void test_report_print_result(void **state) {
}

void test_report_deinit(void **state) {
    report_deinit();
}


// Main function to run all tests
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_report_init),
        cmocka_unit_test(test_report_add_resp_code),
        cmocka_unit_test(test_report_add_req_failure),
        cmocka_unit_test(test_report_print_result),
        cmocka_unit_test(test_report_deinit),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
