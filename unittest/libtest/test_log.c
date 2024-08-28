// test_log.c
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include "worker.h"
#include "common.h"
#include "userdbg.h"

#include "log.c"

// Mock implementations for the socket functions
int __wrap_socket(int domain, int type, int protocol) {
    check_expected(domain);
    check_expected(type);
    check_expected(protocol);
    return mock_type(int);
}

int __wrap_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    check_expected(sockfd);
    check_expected(addr);
    check_expected(addrlen);
    return mock_type(int);
}

ssize_t __wrap_send(int sockfd, const void *buf, size_t len, int flags) {
    check_expected(sockfd);
    check_expected(buf);
    check_expected(len);
    check_expected(flags);
    return mock_type(ssize_t);
}

int __wrap_close(int fd) {
    check_expected(fd);
    return mock_type(int);
}

int __wrap_fork(void) {
    return mock_type(int);
}

pid_t __wrap_waitpid(pid_t pid, int *status, int options) {
    check_expected(pid);
    check_expected(status);
    check_expected(options);
    return mock_type(pid_t);
}

int __wrap_kill(pid_t pid, int sig) {
    check_expected(pid);
    check_expected(sig);
    return mock_type(int);
}

// Unit test for log_init function
static void test_log_init(void **state) {
    (void)state; // unused

    // Set up expectations for socket calls
    expect_value(__wrap_socket, domain, AF_UNIX);
    expect_value(__wrap_socket, type, SOCK_DGRAM);
    expect_value(__wrap_socket, protocol, 0);
    will_return(__wrap_socket, 3);  // Mocked socket descriptor

    // Mocking successful connection
    expect_value(__wrap_connect, sockfd, 3);
    expect_any(__wrap_connect, addr);
    expect_value(__wrap_connect, addrlen, sizeof(struct sockaddr_un));
    will_return(__wrap_connect, 0);

    // Initialize log system
    assert_int_equal(log_init(NULL), 0);

    // Clean up
    log_deinit();
}

// Unit test for start_log_server function
static void test_start_log_server(void **state) {
    (void)state; // unused

    log_client.logserver_pid = 0;  // Ensure log server is not running

    // Mock fork() to return 0 (child process)
    will_return(__wrap_fork, 0);

    // Simulate start_log_server function
    assert_int_equal(start_log_server(), 0);

    // Mock fork() to return a positive PID (parent process)
    will_return(__wrap_fork, 1234);
    log_client.logserver_pid = 0;

    // Simulate starting the server
    assert_int_equal(start_log_server(), 0);
    assert_int_equal(log_client.logserver_pid, 1234);
}

// Unit test for stop_log_server function
static void test_stop_log_server(void **state) {
    (void)state; // unused

    log_client.logserver_pid = 1234;

    // Mock the kill function
    expect_value(__wrap_kill, pid, log_client.logserver_pid);
    expect_value(__wrap_kill, sig, SIGTERM);
    will_return(__wrap_kill, 0);

    // Mock waitpid function
    expect_value(__wrap_waitpid, pid, log_client.logserver_pid);
    expect_any(__wrap_waitpid, status);
    expect_value(__wrap_waitpid, options, 0);
    will_return(__wrap_waitpid, log_client.logserver_pid);

    // Call the stop_log_server function
    stop_log_server();
    assert_int_equal(log_client.logserver_pid, -1);
}

// Unit test for log_write function
static void test_log_write(void **state) {
    (void)state; // unused

    log_client.is_initialized = LOG_INIT_STATUS_INITIALIZED;
    char test_data[] = "Test log data";

    // Write to log buffer
    log_write(test_data, strlen(test_data));

    // Assert that the buffer contains the written data
    assert_true(buffer_contains(&log_client.buffer, test_data));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_log_init),
        cmocka_unit_test(test_start_log_server),
        cmocka_unit_test(test_stop_log_server),
        cmocka_unit_test(test_log_write),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
