#ifndef KSEDIT_TEST_H
#define KSEDIT_TEST_H

#include <stdio.h>
#include <string.h>

static int tests_run    = 0;
static int tests_passed = 0;

#define TEST(name) static void name(void)

#define ASSERT(cond)                                                     \
    do {                                                                 \
        if (!(cond)) {                                                   \
            printf("  FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            return;                                                      \
        }                                                                \
    } while (0)

#define ASSERT_EQ(a, b)                                                  \
    do {                                                                 \
        if ((a) != (b)) {                                                \
            printf("  FAIL: %s:%d: %s != %s\n", __FILE__, __LINE__, #a, #b); \
            return;                                                      \
        }                                                                \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                              \
    do {                                                                 \
        if (strcmp((a), (b)) != 0) {                                     \
            printf("  FAIL: %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); \
            return;                                                      \
        }                                                                \
    } while (0)

#define RUN_TEST(name)                                                   \
    do {                                                                 \
        tests_run++;                                                     \
        printf("  %s... ", #name);                                       \
        name();                                                          \
        tests_passed++;                                                  \
        printf("OK\n");                                                  \
    } while (0)

#define TEST_SUMMARY()                                                   \
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);           \
    return tests_passed == tests_run ? 0 : 1

#endif
