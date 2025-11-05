#pragma once
#include <Arduino.h>
#include "config.h"
#include "error_codes.h"

/**
 * @file validation.h
 * @brief Input validation utilities
 */

namespace Validation {
    /**
     * @brief Validation result structure
     */
    struct ValidationResult {
        bool valid;
        ErrorCode errorCode;

        ValidationResult(bool v, ErrorCode code = ErrorCode::SUCCESS)
            : valid(v), errorCode(code) {}
    };

    /**
     * @brief Validate message for BLE keyboard transmission
     * @param msg Message to validate
     * @return ValidationResult with validity and error code
     *
     * Checks:
     * - Message is not empty
     * - Message doesn't exceed maximum length
     * - Message doesn't contain invalid control characters
     */
    inline ValidationResult validateMessage(const String& msg) {
        // Check for empty message
        if (msg.length() == 0) {
            return ValidationResult(false, ErrorCode::MESSAGE_EMPTY);
        }

        // Check maximum length
        if (msg.length() > Config::BLE::MAX_MESSAGE_LENGTH) {
            return ValidationResult(false, ErrorCode::MESSAGE_TOO_LONG);
        }

        // Check for invalid characters (optional - restrict to printable + whitespace)
        // Note: This is a basic check. Adjust based on your security requirements.
        for (size_t i = 0; i < msg.length(); i++) {
            char c = msg.charAt(i);
            // Allow printable ASCII (32-126), newline, carriage return, tab
            if (c < 32 && c != '\n' && c != '\r' && c != '\t') {
                return ValidationResult(false, ErrorCode::INVALID_CHARACTERS);
            }
            // Disallow extended ASCII control characters (127-159)
            if (c == 127 || (c >= 128 && c <= 159)) {
                return ValidationResult(false, ErrorCode::INVALID_CHARACTERS);
            }
        }

        return ValidationResult(true, ErrorCode::SUCCESS);
    }

    /**
     * @brief Sanitize message for safe logging
     * @param msg Message to sanitize
     * @param maxLength Maximum length to log (default 50)
     * @return Sanitized string safe for logging
     */
    inline String sanitizeForLog(const String& msg, size_t maxLength = 50) {
        String sanitized;
        size_t len = min(msg.length(), maxLength);

        for (size_t i = 0; i < len; i++) {
            char c = msg.charAt(i);
            if (c >= 32 && c <= 126) {
                sanitized += c;
            } else if (c == '\n') {
                sanitized += "\\n";
            } else if (c == '\r') {
                sanitized += "\\r";
            } else if (c == '\t') {
                sanitized += "\\t";
            } else {
                sanitized += '.';
            }
        }

        if (msg.length() > maxLength) {
            sanitized += "...";
        }

        return sanitized;
    }
}
