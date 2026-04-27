#ifndef CXLSB_TEST_FRAMEWORK_H
#define CXLSB_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TEST_PASS 0
#define TEST_FAIL 1

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    printf("Running test: %s...\n", #name); \
    int result = test_##name(); \
    if (result == TEST_PASS) { \
        printf("  PASSED\n"); \
        passed++; \
    } else { \
        printf("  FAILED\n"); \
        failed++; \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("  Assertion failed: %s at line %d\n", #cond, __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("  Assertion failed: %s != %s at line %d\n", #a, #b, __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        printf("  Assertion failed: string '%s' != '%s' at line %d\n", a, b, __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_NOT_NULL(p) do { \
    if ((p) == NULL) { \
        printf("  Assertion failed: %s is NULL at line %d\n", #p, __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#define ASSERT_NULL(p) do { \
    if ((p) != NULL) { \
        printf("  Assertion failed: %s is not NULL at line %d\n", #p, __LINE__); \
        return TEST_FAIL; \
    } \
} while(0)

#endif