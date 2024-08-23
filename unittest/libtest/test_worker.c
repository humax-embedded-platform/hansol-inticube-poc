#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "worker.h"

void mock_task_handler(void *arg) {
    (void)arg;
}

static void test_worker_init_valid(void **state) {
    (void)state;

    task_t task = {
        .task_handler = mock_task_handler,
        .arg = NULL
    };
    worker_t worker;

    int result = worker_init(&worker, &task);
    assert_int_equal(result, 0);
    assert_true(worker.active);
    
    // Clean up
    worker_deinit(&worker);
}

static void test_worker_init_null_task_handler(void **state) {
    (void)state;

    task_t task = {
        .task_handler = NULL,
        .arg = NULL
    };
    worker_t worker;

    int result = worker_init(&worker, &task);
    assert_int_equal(result, -1);
    assert_false(worker.active);
}

static void test_worker_deinit_active_worker(void **state) {
    (void)state;

    task_t task = {
        .task_handler = mock_task_handler,
        .arg = NULL
    };
    worker_t worker;
    worker_init(&worker, &task);

    worker_deinit(&worker);
    assert_false(worker.active);
}

static void test_worker_deinit_inactive_worker(void **state) {
    (void)state;

    task_t task = {
        .task_handler = mock_task_handler,
        .arg = NULL
    };
    worker_t worker = {
        .active = false
    };

    worker_deinit(&worker);
    assert_false(worker.active);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_worker_init_valid),
        cmocka_unit_test(test_worker_init_null_task_handler),
        cmocka_unit_test(test_worker_deinit_active_worker),
        cmocka_unit_test(test_worker_deinit_inactive_worker),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
