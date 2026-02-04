#ifndef KSEDIT_TEST_H
#define KSEDIT_TEST_H

#include <stdio.h>
#include <string.h>

static int tests_run    = 0;
static int tests_passed = 0;
static int current_test_failed = 0;

#define TEST(name) static void name(void)

#define ASSERT(cond)                                                     \
    do {                                                                 \
        if (!(cond)) {                                                   \
            printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            current_test_failed = 1;                                     \
            return;                                                      \
        }                                                                \
    } while (0)

#define ASSERT_EQ(a, b)                                                  \
    do {                                                                 \
        long long _a = (long long)(a);                                   \
        long long _b = (long long)(b);                                   \
        if (_a != _b) {                                                  \
            printf("FAIL: %s:%d: %s != %s (%lld != %lld)\n",             \
                   __FILE__, __LINE__, #a, #b, _a, _b);                  \
            current_test_failed = 1;                                     \
            return;                                                      \
        }                                                                \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                              \
    do {                                                                 \
        if (strcmp((a), (b)) != 0) {                                     \
            printf("FAIL: %s:%d: \"%s\" != \"%s\"\n",                    \
                   __FILE__, __LINE__, (a), (b));                        \
            current_test_failed = 1;                                     \
            return;                                                      \
        }                                                                \
    } while (0)

#define RUN_TEST(name)                                                   \
    do {                                                                 \
        tests_run++;                                                     \
        current_test_failed = 0;                                         \
        printf("  %s... ", #name);                                       \
        name();                                                          \
        if (!current_test_failed) {                                      \
            tests_passed++;                                              \
            printf("OK\n");                                              \
        }                                                                \
    } while (0)

#define TEST_SUMMARY()                                                   \
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);           \
    return tests_passed == tests_run ? 0 : 1

#endif
