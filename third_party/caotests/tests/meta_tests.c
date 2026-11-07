#include "../src/test_lib.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

//get_current_memory_bytes defined in test_lib.c
extern long long get_current_memory_bytes(void);

//helper functions: we wrap macros so that their result
//can be checked. Convention: the helper returns 0 if
//the assertion PASSED (condition met) and 1 if it FAILED.

static int helper_eq_ok(void)    { CT_ASSERT_EQ(2 + 2, 4);  return 0; }
static int helper_eq_fail(void)  { CT_ASSERT_EQ(2 + 2, 5);  return 0; } //return 0 unreachable on failure

static int helper_eq_negative(void) { CT_ASSERT_EQ(-5, -5);  return 0; }
static int helper_eq_expr(void)     { CT_ASSERT_EQ(5, 2 * 3 - 1); return 0; }

static int helper_true_ok(void)   { CT_ASSERT_TRUE(1);       return 0; }
static int helper_true_fail(void) { CT_ASSERT_TRUE(0);       return 0; }

static int helper_false_ok(void)   { CT_ASSERT_FALSE(0);     return 0; }
static int helper_false_fail(void) { CT_ASSERT_FALSE(1);     return 0; }

//a function guaranteed to fail, for testing ct_run_test
static int always_failing_test(void) { return 1; }
static int always_passing_test(void) { return 0; }

//function with a delay, for testing time measurement.
static int slow_test(void) {
    struct timespec ts = {0, 50 * 1000 * 1000}; //50 ms
    nanosleep(&ts, NULL);
    return 0;
}

//meta tests

//the test section is registered and contains at least this test.
CT_TEST(section_is_not_empty) {
    long count = &__stop_ct_tests - &__start_ct_tests;
    CT_ASSERT_TRUE(count >= 1);
    return 0;
}

//the CT_ASSERT_EQ macro passes when the values are equal
CT_TEST(assert_eq_passes_on_equal) {
    CT_ASSERT_EQ(0, helper_eq_ok());
    return 0;
}

//the CT_ASSERT_EQ macro fails on unequal values
CT_TEST(assert_eq_fails_on_unequal) {
    CT_ASSERT_EQ(1, helper_eq_fail());
    return 0;
}

//works with negative numbers and expressions
CT_TEST(assert_eq_handles_negatives_and_expr) {
    CT_ASSERT_EQ(0, helper_eq_negative());
    CT_ASSERT_EQ(0, helper_eq_expr());
    return 0;
}

//CT_ASSERT_TRUE passes on true and fails on false
CT_TEST(assert_true_behavior) {
    CT_ASSERT_EQ(0, helper_true_ok());
    CT_ASSERT_EQ(1, helper_true_fail());
    return 0;
}

//CT_ASSERT_FALSE passes on false and fails on true
CT_TEST(assert_false_behavior) {
    CT_ASSERT_EQ(0, helper_false_ok());
    CT_ASSERT_EQ(1, helper_false_fail());
    return 0;
}

//ct_run_test returns 0 for a passing test
CT_TEST(run_test_returns_zero_on_pass) {
    CT_ASSERT_EQ(0, ct_run_test(always_passing_test, "dummy_pass", __FILE__, -1));
    return 0;
}

//ct_run_test returns 1 for a failing test
CT_TEST(run_test_returns_one_on_fail) {
    CT_ASSERT_EQ(1, ct_run_test(always_failing_test, "dummy_fail", __FILE__, -1));
    return 0;
}

//the memory measurement returns a reasonable (positive) value
CT_TEST(memory_measurement_positive) {
    long long mem = get_current_memory_bytes();
    CT_ASSERT_TRUE(mem > 0);
    return 0;
}

//the measured memory usage does not decrease after memory allocation
//caution: the figures depend on the malloc implementation,
//so we only check that it is "not less"
CT_TEST(memory_grows_after_alloc) {
    long long before = get_current_memory_bytes();
    void *p = malloc(1024 * 1024); /* 1 МБ */
    long long after = get_current_memory_bytes();
    free(p);
    CT_ASSERT_TRUE(after >= before);
    return 0;
}
