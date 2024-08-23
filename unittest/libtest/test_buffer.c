#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "buffer.h"
// Test case for buffer initialization
static void test_buffer_init(void **state) {
    (void)state; // unused variable

    buffer_t buf;
    buffer_init(&buf, 10);

    assert_int_equal(buf.capacity, 10);
    assert_int_equal(buf.head, 0);
    assert_int_equal(buf.tail, 0);
    assert_int_equal(buf.count, 0);
    assert_non_null(buf.entries);

    for (size_t i = 0; i < buf.capacity; i++) {
        assert_null(buf.entries[i].data);
        assert_int_equal(buf.entries[i].size, 0);
        assert_int_equal(buf.entries[i].retry, 0);
    }

    buffer_deinit(&buf);
}

// Test case for writing data to buffer
static void test_buffer_write(void **state) {
    (void)state; // unused variable

    buffer_t buf;
    buffer_init(&buf, 10);

    const char *data = "Hello, world!";
    buffer_write(&buf, data, strlen(data) + 1);

    assert_int_equal(buf.count, 1);
    assert_int_equal(buf.head, 0);
    assert_int_equal(buf.tail, 1);

    log_entry_t log;
    buffer_read(&buf, &log);
    assert_string_equal(log.data, data);
    assert_int_equal(log.size, strlen(data) + 1);
    assert_int_equal(log.retry, 0);

    buffer_log_entry_deinit(&log);
    buffer_deinit(&buf);
}

static void test_buffer_write_with_retry(void **state) {
    (void)state;

    buffer_t buf;
    buffer_init(&buf, 3);

    const char *data1 = "Data1";
    const char *data2 = "Data2";
    const char *data3 = "Data3";
    const char *data4 = "Data4";

    buffer_write_with_retry(&buf, data1, strlen(data1) + 1, 1);
    buffer_write_with_retry(&buf, data2, strlen(data2) + 1, 2);
    buffer_write_with_retry(&buf, data3, strlen(data3) + 1, 3);

    buffer_write_with_retry(&buf, data4, strlen(data4) + 1, 4);

    assert_int_equal(buf.count, 3);
    assert_int_equal(buf.head, 1);
    assert_int_equal(buf.tail, 1);

    log_entry_t log;
    buffer_read(&buf, &log);
    assert_string_equal(log.data, data2);
    assert_int_equal(log.size, strlen(data2) + 1);
    assert_int_equal(log.retry, 2);

    buffer_log_entry_deinit(&log);
    buffer_read(&buf, &log);
    assert_string_equal(log.data, data3);
    assert_int_equal(log.size, strlen(data3) + 1);
    assert_int_equal(log.retry, 3);

    buffer_log_entry_deinit(&log);
    buffer_read(&buf, &log);
    assert_string_equal(log.data, data4);
    assert_int_equal(log.size, strlen(data4) + 1);
    assert_int_equal(log.retry, 4);

    buffer_log_entry_deinit(&log);
    buffer_deinit(&buf);
}

static void test_buffer_read_empty(void **state) {
    (void)state;

    buffer_t buf;
    buffer_init(&buf, 10);

    log_entry_t log;
    int result = buffer_read(&buf, &log);

    assert_int_equal(result, -1);

    buffer_deinit(&buf);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_buffer_init),
        cmocka_unit_test(test_buffer_write),
        cmocka_unit_test(test_buffer_write_with_retry),
        cmocka_unit_test(test_buffer_read_empty),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
