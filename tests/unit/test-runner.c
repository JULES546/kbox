/* SPDX-License-Identifier: MIT */
/*
 * Minimal C test harness.  No external dependencies.
 *
 * Each test file registers tests via TEST_REGISTER() in its
 * init function.  test-runner.c calls all init functions, then
 * runs all registered tests.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TESTS 256

typedef void (*test_fn)(void);

struct test_entry {
    const char *name;
    test_fn fn;
};

static struct test_entry tests[MAX_TESTS];
static int test_count;
static int pass_count;
static int fail_count;
static const char *current_test;

void test_register(const char *name, test_fn fn)
{
    if (test_count >= MAX_TESTS) {
        fprintf(stderr, "too many tests (max %d)\n", MAX_TESTS);
        exit(1);
    }
    tests[test_count].name = name;
    tests[test_count].fn = fn;
    test_count++;
}

void test_fail(const char *file, int line, const char *expr)
{
    fprintf(stderr, "  FAIL: %s:%d: %s\n", file, line, expr);
    fail_count++;
}

void test_pass(void)
{
    pass_count++;
}

#define ASSERT_TRUE(expr)                         \
    do {                                          \
        if (!(expr)) {                            \
            test_fail(__FILE__, __LINE__, #expr); \
            return;                               \
        }                                         \
        test_pass();                              \
    } while (0)

#define ASSERT_EQ(a, b)                                               \
    do {                                                              \
        long _a = (long) (a), _b = (long) (b);                        \
        if (_a != _b) {                                               \
            fprintf(stderr, "  FAIL: %s:%d: %s == %s (%ld != %ld)\n", \
                    __FILE__, __LINE__, #a, #b, _a, _b);              \
            fail_count++;                                             \
            return;                                                   \
        }                                                             \
        test_pass();                                                  \
    } while (0)

#define ASSERT_NE(a, b)                                                       \
    do {                                                                      \
        long _a = (long) (a), _b = (long) (b);                                \
        if (_a == _b) {                                                       \
            fprintf(stderr, "  FAIL: %s:%d: %s != %s (both %ld)\n", __FILE__, \
                    __LINE__, #a, #b, _a);                                    \
            fail_count++;                                                     \
            return;                                                           \
        }                                                                     \
        test_pass();                                                          \
    } while (0)

#define ASSERT_STREQ(a, b)                                                 \
    do {                                                                   \
        const char *_a = (a), *_b = (b);                                   \
        if (strcmp(_a, _b) != 0) {                                         \
            fprintf(stderr, "  FAIL: %s:%d: \"%s\" != \"%s\"\n", __FILE__, \
                    __LINE__, _a, _b);                                     \
            fail_count++;                                                  \
            return;                                                        \
        }                                                                  \
        test_pass();                                                       \
    } while (0)

#define TEST_REGISTER(fn) test_register(#fn, fn)

/* Include the shared test header for all test files */
#include "test-runner.h"

/* External init functions from each test file */
extern void test_fd_table_init(void);
extern void test_path_init(void);
extern void test_identity_init(void);
extern void test_syscall_nr_init(void);
extern void test_elf_init(void);

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    /* Register all test suites */
    test_fd_table_init();
    test_path_init();
    test_identity_init();
    test_syscall_nr_init();
    test_elf_init();

    /* Run all tests */
    int suite_fails = 0;
    printf("Running %d tests...\n", test_count);
    for (int i = 0; i < test_count; i++) {
        current_test = tests[i].name;
        int before_fail = fail_count;
        printf("  [%d/%d] %s... ", i + 1, test_count, tests[i].name);
        fflush(stdout);
        tests[i].fn();
        if (fail_count > before_fail) {
            suite_fails++;
            printf("FAILED\n");
        } else {
            printf("ok\n");
        }
    }

    printf("\n%d tests, %d passed, %d failed\n", test_count,
           test_count - suite_fails, suite_fails);

    return suite_fails > 0 ? 1 : 0;
}
