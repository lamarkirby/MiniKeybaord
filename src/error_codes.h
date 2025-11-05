#pragma once

/**
 * @file error_codes.h
 * @brief Standardized error codes for the application
 */

enum class ErrorCode {
    SUCCESS = 0,
    BLE_NOT_CONNECTED = 1,
    BLE_SEND_FAILED = 2,
    WIFI_NOT_CONNECTED = 3,
    MESSAGE_TOO_LONG = 4,
    MESSAGE_EMPTY = 5,
    INVALID_PARAMETER = 6,
    INVALID_CHARACTERS = 7,
    RATE_LIMIT_EXCEEDED = 8,
    UNAUTHORIZED = 9,
    BUSY = 10,
    INTERNAL_ERROR = 99
};

/**
 * @brief Get human-readable error message
 * @param code Error code
 * @return Error message string
 */
inline const char* errorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS:
            return "Success";
        case ErrorCode::BLE_NOT_CONNECTED:
            return "BLE keyboard not connected";
        case ErrorCode::BLE_SEND_FAILED:
            return "Failed to send via BLE";
        case ErrorCode::WIFI_NOT_CONNECTED:
            return "WiFi not connected";
        case ErrorCode::MESSAGE_TOO_LONG:
            return "Message exceeds maximum length";
        case ErrorCode::MESSAGE_EMPTY:
            return "Message cannot be empty";
        case ErrorCode::INVALID_PARAMETER:
            return "Invalid parameter";
        case ErrorCode::INVALID_CHARACTERS:
            return "Message contains invalid characters";
        case ErrorCode::RATE_LIMIT_EXCEEDED:
            return "Rate limit exceeded - too many requests";
        case ErrorCode::UNAUTHORIZED:
            return "Unauthorized - valid API key required";
        case ErrorCode::BUSY:
            return "System busy - another operation in progress";
        default:
            return "Internal error";
    }
}

/**
 * @brief Get HTTP status code for error
 * @param code Error code
 * @return HTTP status code
 */
inline uint16_t httpStatusCode(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS:
            return 200;
        case ErrorCode::UNAUTHORIZED:
            return 401;
        case ErrorCode::RATE_LIMIT_EXCEEDED:
            return 429;
        case ErrorCode::BUSY:
            return 409;
        case ErrorCode::BLE_NOT_CONNECTED:
        case ErrorCode::MESSAGE_TOO_LONG:
        case ErrorCode::MESSAGE_EMPTY:
        case ErrorCode::INVALID_PARAMETER:
        case ErrorCode::INVALID_CHARACTERS:
            return 400;
        default:
            return 500;
    }
}
