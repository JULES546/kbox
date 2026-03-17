/* SPDX-License-Identifier: MIT */
#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

/*
 * Shared test macros.  Included by test_runner.c which defines
 * the actual implementations; individual test files include
 * this header for the macros.
 */

typedef void (*test_fn)(void);

void test_register(const char *name, test_fn fn);
void test_fail(const char *file, int line, const char *expr);
void test_pass(void);

#ifndef ASSERT_TRUE
#define ASSERT_TRUE(expr)                         \
    do {                                          \
        if (!(expr)) {                            \
            test_fail(__FILE__, __LINE__, #expr); \
            return;                               \
        }                                         \
        test_pass();                              \
    } while (0)
#endif

#ifndef ASSERT_EQ
#define ASSERT_EQ(a, b)                                  \
    do {                                                 \
        long _a = (long) (a), _b = (long) (b);           \
        if (_a != _b) {                                  \
            test_fail(__FILE__, __LINE__, #a " == " #b); \
            return;                                      \
        }                                                \
        test_pass();                                     \
    } while (0)
#endif

#ifndef ASSERT_NE
#define ASSERT_NE(a, b)                                  \
    do {                                                 \
        long _a = (long) (a), _b = (long) (b);           \
        if (_a == _b) {                                  \
            test_fail(__FILE__, __LINE__, #a " != " #b); \
            return;                                      \
        }                                                \
        test_pass();                                     \
    } while (0)
#endif

#ifndef ASSERT_STREQ
#define ASSERT_STREQ(a, b)                                  \
    do {                                                    \
        const char *_a = (a), *_b = (b);                    \
        if (strcmp(_a, _b) != 0) {                          \
            test_fail(__FILE__, __LINE__, #a " streq " #b); \
            return;                                         \
        }                                                   \
        test_pass();                                        \
    } while (0)
#endif

#define TEST_REGISTER(fn) test_register(#fn, fn)

#endif /* TEST_RUNNER_H */
