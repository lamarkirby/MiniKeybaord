#include <unity.h>
#include "mocks/Arduino.h"
#include "utils/time_utils.h"

/**
 * @file test_time_utils.cpp
 * @brief Unit tests for overflow-safe timing utilities
 *
 * These tests verify that time comparisons work correctly even when
 * millis() overflows after ~49 days (4,294,967,295 milliseconds).
 *
 * THIS IS CRITICAL - These tests validate BUG-001 and BUG-003 fixes!
 */

void setUp(void) {
    // Reset mock time before each test
    mock_millis_value = 0;
}

void tearDown(void) {
    // Cleanup after each test
}

// ===== hasElapsed() Tests =====

// Test: Basic case - time has elapsed
void test_hasElapsed_normal_elapsed() {
    unsigned long start = 1000;
    mock_millis_value = 1600;  // 600ms later

    // 500ms has elapsed
    TEST_ASSERT_TRUE(TimeUtils::hasElapsed(start, 500));
}

// Test: Basic case - time has not elapsed
void test_hasElapsed_normal_not_elapsed() {
    unsigned long start = 1000;
    mock_millis_value = 1400;  // 400ms later

    // 500ms has NOT elapsed
    TEST_ASSERT_FALSE(TimeUtils::hasElapsed(start, 500));
}

// Test: Exact boundary - time equals interval
void test_hasElapsed_exact_boundary() {
    unsigned long start = 1000;
    mock_millis_value = 1500;  // Exactly 500ms later

    // Exactly 500ms should return true
    TEST_ASSERT_TRUE(TimeUtils::hasElapsed(start, 500));
}

// Test: CRITICAL - millis() overflow case
void test_hasElapsed_overflow_case() {
    // Simulate near-overflow scenario
    // Start time is 100ms before overflow
    unsigned long start = 4294967195UL;  // 4,294,967,295 - 100

    // millis() has wrapped around to 500
    mock_millis_value = 500;

    // Total time elapsed: (4,294,967,295 - 4,294,967,195) + 500 = 600ms
    TEST_ASSERT_TRUE(TimeUtils::hasElapsed(start, 500));
    TEST_ASSERT_FALSE(TimeUtils::hasElapsed(start, 700));
}

// Test: CRITICAL - Large overflow scenario
void test_hasElapsed_large_overflow() {
    // Start near max value
    unsigned long start = 4294960000UL;

    // Wrapped around significantly
    mock_millis_value = 10000;

    // Time elapsed: (4,294,967,295 - 4,294,960,000) + 10,000 = 17,295ms
    TEST_ASSERT_TRUE(TimeUtils::hasElapsed(start, 15000));
    TEST_ASSERT_FALSE(TimeUtils::hasElapsed(start, 20000));
}

// Test: Zero interval should always return true
void test_hasElapsed_zero_interval() {
    unsigned long start = 1000;
    mock_millis_value = 1000;

    TEST_ASSERT_TRUE(TimeUtils::hasElapsed(start, 0));
}

// ===== timeDiff() Tests =====

// Test: Basic time difference
void test_timeDiff_normal() {
    unsigned long start = 1000;
    mock_millis_value = 2500;

    TEST_ASSERT_EQUAL(1500, TimeUtils::timeDiff(start));
}

// Test: Zero difference
void test_timeDiff_zero() {
    unsigned long start = 1000;
    mock_millis_value = 1000;

    TEST_ASSERT_EQUAL(0, TimeUtils::timeDiff(start));
}

// Test: CRITICAL - Time difference with overflow
void test_timeDiff_overflow() {
    unsigned long start = 4294967000UL;
    mock_millis_value = 1000;

    // Time diff: (4,294,967,295 - 4,294,967,000) + 1,000 = 1,295ms
    unsigned long diff = TimeUtils::timeDiff(start);

    TEST_ASSERT_EQUAL(1295, diff);
}

// Test: Time difference with specified end time
void test_timeDiff_with_end_time() {
    unsigned long start = 1000;
    unsigned long end = 2500;

    TEST_ASSERT_EQUAL(1500, TimeUtils::timeDiff(start, end));
}

// ===== withinWindow() Tests =====

// Test: Within time window
void test_withinWindow_inside() {
    unsigned long timestamp = 1000;
    mock_millis_value = 1400;  // 400ms later

    TEST_ASSERT_TRUE(TimeUtils::withinWindow(timestamp, 500));
}

// Test: Outside time window
void test_withinWindow_outside() {
    unsigned long timestamp = 1000;
    mock_millis_value = 1600;  // 600ms later

    TEST_ASSERT_FALSE(TimeUtils::withinWindow(timestamp, 500));
}

// Test: Exactly at window boundary
void test_withinWindow_boundary() {
    unsigned long timestamp = 1000;
    mock_millis_value = 1500;  // Exactly 500ms later

    // Should be false (>= window, not strictly <)
    TEST_ASSERT_FALSE(TimeUtils::withinWindow(timestamp, 500));
}

// Test: CRITICAL - Window check with overflow
void test_withinWindow_overflow() {
    unsigned long timestamp = 4294967000UL;
    mock_millis_value = 500;

    // 1,295ms has elapsed, window is 2000ms
    TEST_ASSERT_TRUE(TimeUtils::withinWindow(timestamp, 2000));

    // 1,295ms has elapsed, window is 1000ms
    TEST_ASSERT_FALSE(TimeUtils::withinWindow(timestamp, 1000));
}

// ===== Edge Cases =====

// Test: Maximum possible time value
void test_max_time_value() {
    unsigned long start = 0xFFFFFFFFUL;  // Maximum uint32
    mock_millis_value = 100;

    // Should handle gracefully
    TEST_ASSERT_TRUE(TimeUtils::hasElapsed(start, 50));
}

// Test: Both times at maximum
void test_both_at_max() {
    unsigned long start = 0xFFFFFFFFUL;
    mock_millis_value = 0xFFFFFFFFUL;

    TEST_ASSERT_EQUAL(0, TimeUtils::timeDiff(start));
}

void setup() {
    UNITY_BEGIN();

    // hasElapsed tests
    RUN_TEST(test_hasElapsed_normal_elapsed);
    RUN_TEST(test_hasElapsed_normal_not_elapsed);
    RUN_TEST(test_hasElapsed_exact_boundary);
    RUN_TEST(test_hasElapsed_overflow_case);
    RUN_TEST(test_hasElapsed_large_overflow);
    RUN_TEST(test_hasElapsed_zero_interval);

    // timeDiff tests
    RUN_TEST(test_timeDiff_normal);
    RUN_TEST(test_timeDiff_zero);
    RUN_TEST(test_timeDiff_overflow);
    RUN_TEST(test_timeDiff_with_end_time);

    // withinWindow tests
    RUN_TEST(test_withinWindow_inside);
    RUN_TEST(test_withinWindow_outside);
    RUN_TEST(test_withinWindow_boundary);
    RUN_TEST(test_withinWindow_overflow);

    // Edge cases
    RUN_TEST(test_max_time_value);
    RUN_TEST(test_both_at_max);

    UNITY_END();
}

void loop() {
    // Not used in native testing
}
