/*
 * Minimal host test framework: registration, filtering, assertions.
 */
#ifndef PSG_TESTS_HOST_TEST_SUPPORT_H
#define PSG_TESTS_HOST_TEST_SUPPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*test_fn_t)(void);

void test_register(const char *suite, const char *name, test_fn_t fn);
int test_run_all(const char *filter);

void test_fail(const char *file, int line, const char *expression);
void test_fail_u64(const char *file, int line, const char *expression, uint64_t actual, uint64_t expected);

#define TEST(suite, name)                                                                        \
    static void suite##_##name(void);                                                            \
    __attribute__((constructor)) static void register_##suite##_##name(void)                     \
    {                                                                                            \
        test_register(#suite, #name, suite##_##name);                                            \
    }                                                                                            \
    static void suite##_##name(void)

#define CHECK(condition)                                                                         \
    do {                                                                                         \
        if (!(condition)) {                                                                      \
            test_fail(__FILE__, __LINE__, #condition);                                           \
        }                                                                                        \
    } while (0)

#define CHECK_EQ(actual, expected)                                                               \
    do {                                                                                         \
        uint64_t actual_value = (uint64_t)(actual);                                              \
        uint64_t expected_value = (uint64_t)(expected);                                          \
        if (actual_value != expected_value) {                                                    \
            test_fail_u64(__FILE__, __LINE__, #actual " == " #expected, actual_value, expected_value); \
        }                                                                                        \
    } while (0)

#define CHECK_NEAR(actual, expected, epsilon)                                                    \
    do {                                                                                         \
        double actual_value = (double)(actual);                                                  \
        double expected_value = (double)(expected);                                              \
        double delta = actual_value - expected_value;                                            \
        if (!((delta <= (epsilon)) && (delta >= -(epsilon)))) {                                   \
            test_fail(__FILE__, __LINE__, #actual " ~= " #expected);                              \
        }                                                                                        \
    } while (0)

#endif /* PSG_TESTS_HOST_TEST_SUPPORT_H */
