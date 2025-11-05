#include <unity.h>
#include "mocks/Arduino.h"
#include <IPAddress.h>
#include "utils/rate_limiter.h"

/**
 * @file test_rate_limiter.cpp
 * @brief Unit tests for rate limiting
 *
 * Tests the rate limiter's ability to track and limit requests
 * from different IP addresses.
 */

// Mock IPAddress if not available
#ifndef IPAddress_h
class IPAddress {
private:
    uint32_t addr;
public:
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        addr = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | d;
    }
    operator uint32_t() const { return addr; }
};
#endif

void setUp(void) {
    mock_millis_value = 0;
}

void tearDown(void) {
    // Cleanup
}

// Test: First request should be allowed
void test_first_request_allowed() {
    RateLimiter limiter(1000, 5);  // 5 requests per second
    IPAddress ip(192, 168, 1, 100);

    TEST_ASSERT_TRUE(limiter.checkLimit(ip));
}

// Test: Multiple requests within limit should be allowed
void test_multiple_within_limit() {
    RateLimiter limiter(1000, 5);
    IPAddress ip(192, 168, 1, 100);

    // First 5 requests should pass
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE_MESSAGE(limiter.checkLimit(ip), "Request within limit should pass");
    }
}

// Test: Exceeding rate limit should be blocked
void test_exceeding_rate_limit() {
    RateLimiter limiter(1000, 5);
    IPAddress ip(192, 168, 1, 100);

    // First 5 pass
    for (int i = 0; i < 5; i++) {
        limiter.checkLimit(ip);
    }

    // 6th request should be blocked
    TEST_ASSERT_FALSE(limiter.checkLimit(ip));
}

// Test: Different IPs tracked separately
void test_different_ips_tracked_separately() {
    RateLimiter limiter(1000, 3);
    IPAddress ip1(192, 168, 1, 100);
    IPAddress ip2(192, 168, 1, 101);

    // IP1: 3 requests
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_TRUE(limiter.checkLimit(ip1));
    }

    // IP1: 4th request blocked
    TEST_ASSERT_FALSE(limiter.checkLimit(ip1));

    // IP2: Should still be allowed (different IP)
    TEST_ASSERT_TRUE(limiter.checkLimit(ip2));
}

// Test: Rate limit window reset
void test_rate_limit_window_reset() {
    RateLimiter limiter(1000, 5);
    IPAddress ip(192, 168, 1, 100);

    mock_millis_value = 0;

    // Use up all 5 requests
    for (int i = 0; i < 5; i++) {
        limiter.checkLimit(ip);
    }

    // 6th request blocked
    TEST_ASSERT_FALSE(limiter.checkLimit(ip));

    // Advance time past window
    mock_millis_value = 1100;

    // Should be allowed again (new window)
    TEST_ASSERT_TRUE(limiter.checkLimit(ip));
}

// Test: Cleanup removes old entries
void test_cleanup_removes_old_entries() {
    RateLimiter limiter(1000, 5);
    IPAddress ip1(192, 168, 1, 100);
    IPAddress ip2(192, 168, 1, 101);

    mock_millis_value = 0;
    limiter.checkLimit(ip1);
    limiter.checkLimit(ip2);

    TEST_ASSERT_EQUAL(2, limiter.getTrackedClientCount());

    // Advance time significantly
    mock_millis_value = 20000;  // 20 seconds later

    // Trigger cleanup
    limiter.cleanup();

    // Old entries should be removed
    TEST_ASSERT_EQUAL(0, limiter.getTrackedClientCount());
}

// Test: Reset clears all tracking
void test_reset_clears_all() {
    RateLimiter limiter(1000, 5);
    IPAddress ip(192, 168, 1, 100);

    limiter.checkLimit(ip);
    TEST_ASSERT_EQUAL(1, limiter.getTrackedClientCount());

    limiter.reset();
    TEST_ASSERT_EQUAL(0, limiter.getTrackedClientCount());
}

// Test: Overflow-safe timing
void test_overflow_safe_timing() {
    RateLimiter limiter(1000, 5);
    IPAddress ip(192, 168, 1, 100);

    // Start near overflow
    mock_millis_value = 4294967000UL;

    // Use requests
    for (int i = 0; i < 5; i++) {
        limiter.checkLimit(ip);
    }
    TEST_ASSERT_FALSE(limiter.checkLimit(ip));

    // Overflow millis
    mock_millis_value = 500;

    // Should reset (new window after overflow)
    TEST_ASSERT_TRUE(limiter.checkLimit(ip));
}

void setup() {
    UNITY_BEGIN();

    RUN_TEST(test_first_request_allowed);
    RUN_TEST(test_multiple_within_limit);
    RUN_TEST(test_exceeding_rate_limit);
    RUN_TEST(test_different_ips_tracked_separately);
    RUN_TEST(test_rate_limit_window_reset);
    RUN_TEST(test_cleanup_removes_old_entries);
    RUN_TEST(test_reset_clears_all);
    RUN_TEST(test_overflow_safe_timing);

    UNITY_END();
}

void loop() {
    // Not used
}
