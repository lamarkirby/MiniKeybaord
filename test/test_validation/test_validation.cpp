#include <unity.h>
#include "mocks/Arduino.h"
#include "utils/validation.h"

/**
 * @file test_validation.cpp
 * @brief Unit tests for input validation
 *
 * Tests all validation rules without needing any hardware.
 * These tests verify critical security and reliability features.
 */

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

// Test: Empty message should be rejected
void test_empty_message_rejected() {
    String emptyMsg = "";
    auto result = Validation::validateMessage(emptyMsg);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(ErrorCode::MESSAGE_EMPTY, result.errorCode);
}

// Test: Message exactly at max length should be accepted
void test_max_length_message_accepted() {
    // Create message with exactly 1000 characters
    char buffer[1001];
    memset(buffer, 'A', 1000);
    buffer[1000] = '\0';
    String maxMsg(buffer);

    auto result = Validation::validateMessage(maxMsg);

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL(ErrorCode::SUCCESS, result.errorCode);
}

// Test: Message over max length should be rejected
void test_too_long_message_rejected() {
    // Create message with 1001 characters (1 over limit)
    char buffer[1002];
    memset(buffer, 'A', 1001);
    buffer[1001] = '\0';
    String tooLongMsg(buffer);

    auto result = Validation::validateMessage(tooLongMsg);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(ErrorCode::MESSAGE_TOO_LONG, result.errorCode);
}

// Test: Valid message should be accepted
void test_valid_message_accepted() {
    String validMsg = "Hello, World! This is a test message.";
    auto result = Validation::validateMessage(validMsg);

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL(ErrorCode::SUCCESS, result.errorCode);
}

// Test: Message with invalid control characters should be rejected
void test_invalid_control_chars_rejected() {
    // Create string with null terminator in middle (control char)
    char buffer[20] = "Hello";
    buffer[5] = 0x01;  // Control character
    buffer[6] = 'W';
    buffer[7] = 'o';
    buffer[8] = 'r';
    buffer[9] = 'l';
    buffer[10] = 'd';
    buffer[11] = '\0';
    String invalidMsg(buffer);

    auto result = Validation::validateMessage(invalidMsg);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(ErrorCode::INVALID_CHARACTERS, result.errorCode);
}

// Test: Newline should be allowed
void test_newline_allowed() {
    String msgWithNewline = "Line 1\nLine 2";
    auto result = Validation::validateMessage(msgWithNewline);

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL(ErrorCode::SUCCESS, result.errorCode);
}

// Test: Tab should be allowed
void test_tab_allowed() {
    String msgWithTab = "Column1\tColumn2";
    auto result = Validation::validateMessage(msgWithTab);

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL(ErrorCode::SUCCESS, result.errorCode);
}

// Test: Carriage return should be allowed
void test_carriage_return_allowed() {
    String msgWithCR = "Line 1\rLine 2";
    auto result = Validation::validateMessage(msgWithCR);

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL(ErrorCode::SUCCESS, result.errorCode);
}

// Test: Sanitize for log should truncate long messages
void test_sanitize_truncates_long_messages() {
    char buffer[101];
    memset(buffer, 'A', 100);
    buffer[100] = '\0';
    String longMsg(buffer);

    String sanitized = Validation::sanitizeForLog(longMsg, 50);

    TEST_ASSERT_TRUE(sanitized.length() <= 53);  // 50 chars + "..."
    TEST_ASSERT_TRUE(strstr(sanitized.c_str(), "...") != nullptr);
}

// Test: Sanitize should replace control characters
void test_sanitize_replaces_control_chars() {
    String msgWithNewline = "Hello\nWorld";
    String sanitized = Validation::sanitizeForLog(msgWithNewline);

    TEST_ASSERT_TRUE(strstr(sanitized.c_str(), "\\n") != nullptr);
}

void setup() {
    UNITY_BEGIN();

    RUN_TEST(test_empty_message_rejected);
    RUN_TEST(test_max_length_message_accepted);
    RUN_TEST(test_too_long_message_rejected);
    RUN_TEST(test_valid_message_accepted);
    RUN_TEST(test_invalid_control_chars_rejected);
    RUN_TEST(test_newline_allowed);
    RUN_TEST(test_tab_allowed);
    RUN_TEST(test_carriage_return_allowed);
    RUN_TEST(test_sanitize_truncates_long_messages);
    RUN_TEST(test_sanitize_replaces_control_chars);

    UNITY_END();
}

void loop() {
    // Not used in native testing
}
