#include "test_support.h"

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#define TEST_CAPACITY 256

typedef struct {
    const char *suite;
    const char *name;
    test_fn_t fn;
} test_entry_t;

static test_entry_t g_tests[TEST_CAPACITY];
static size_t g_test_count;
static jmp_buf g_abort;
static bool g_abort_armed;
static unsigned g_failures;

void test_register(const char *suite, const char *name, test_fn_t fn)
{
    if (g_test_count >= TEST_CAPACITY) {
        (void)fprintf(stderr, "test registry full\n");
        return;
    }
    g_tests[g_test_count].suite = suite;
    g_tests[g_test_count].name = name;
    g_tests[g_test_count].fn = fn;
    g_test_count++;
}

void test_fail(const char *file, int line, const char *expression)
{
    g_failures++;
    (void)fprintf(stderr, "    FAIL %s:%d: %s\n", file, line, expression);
    if (g_abort_armed) {
        longjmp(g_abort, 1);
    }
}

void test_fail_u64(const char *file, int line, const char *expression, uint64_t actual, uint64_t expected)
{
    g_failures++;
    (void)fprintf(stderr, "    FAIL %s:%d: %s (actual 0x%llX, expected 0x%llX)\n", file, line, expression,
                  (unsigned long long)actual, (unsigned long long)expected);
    if (g_abort_armed) {
        longjmp(g_abort, 1);
    }
}

int test_run_all(const char *filter)
{
    size_t index;
    size_t run = 0u;
    size_t failed = 0u;

    for (index = 0u; index < g_test_count; index++) {
        char full_name[192];
        int result;

        (void)snprintf(full_name, sizeof(full_name), "%s.%s", g_tests[index].suite, g_tests[index].name);
        if ((filter != NULL) && (strstr(full_name, filter) == NULL)) {
            continue;
        }
        run++;
        (void)printf("[ RUN      ] %s\n", full_name);
        g_abort_armed = true;
        result = setjmp(g_abort);
        if (result == 0) {
            g_tests[index].fn();
        }
        g_abort_armed = false;
        if (result == 0) {
            (void)printf("[       OK ] %s\n", full_name);
        } else {
            failed++;
            (void)printf("[  FAILED  ] %s\n", full_name);
        }
    }

    (void)printf("\n%zu test(s), %zu failed, %u assertion failure(s)\n", run, failed, g_failures);
    return (failed == 0u) ? 0 : 1;
}

int main(int argc, char **argv)
{
    const char *filter = (argc > 1) ? argv[1] : NULL;

    (void)setvbuf(stdout, NULL, _IONBF, 0);
    (void)setvbuf(stderr, NULL, _IONBF, 0);
    return test_run_all(filter);
}
