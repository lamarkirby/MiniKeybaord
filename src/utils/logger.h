#pragma once
#include <Arduino.h>
#include "config.h"

/**
 * @file logger.h
 * @brief Logging utility with configurable log levels
 *
 * Provides leveled logging with timestamps and conditional compilation
 * to reduce code size in production builds.
 *
 * Usage:
 *   LOG_DEBUG("Detailed debug information");
 *   LOG_INFO("Normal operational message");
 *   LOG_ERROR("Error occurred");
 *
 *   // With formatting:
 *   LOG_INFO_F("Connected to WiFi: %s", ssid);
 *   LOG_ERROR_F("Failed with code: %d", errorCode);
 */

class Logger {
private:
    /**
     * @brief Format timestamp string
     * @param buffer Buffer to write timestamp to
     * @param bufferSize Size of buffer
     */
    static void formatTimestamp(char* buffer, size_t bufferSize) {
        unsigned long ms = millis();
        unsigned long seconds = ms / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;

        snprintf(buffer, bufferSize, "[%02lu:%02lu:%02lu.%03lu]",
                 hours % 24,
                 minutes % 60,
                 seconds % 60,
                 ms % 1000);
    }

public:
    /**
     * @brief Log debug message
     * @param message Message to log
     */
    static void debug(const char* message) {
        if (Config::Logging::LOG_LEVEL >= Config::Logging::DEBUG) {
            char timestamp[20];
            formatTimestamp(timestamp, sizeof(timestamp));
            Serial.printf("%s [DEBUG] %s\n", timestamp, message);
        }
    }

    /**
     * @brief Log debug message with formatting
     * @param format Printf-style format string
     * @param ... Format arguments
     */
    static void debugF(const char* format, ...) {
        if (Config::Logging::LOG_LEVEL >= Config::Logging::DEBUG) {
            char timestamp[20];
            formatTimestamp(timestamp, sizeof(timestamp));

            Serial.printf("%s [DEBUG] ", timestamp);

            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            Serial.println(buffer);
        }
    }

    /**
     * @brief Log info message
     * @param message Message to log
     */
    static void info(const char* message) {
        if (Config::Logging::LOG_LEVEL >= Config::Logging::INFO) {
            char timestamp[20];
            formatTimestamp(timestamp, sizeof(timestamp));
            Serial.printf("%s [INFO]  %s\n", timestamp, message);
        }
    }

    /**
     * @brief Log info message with formatting
     * @param format Printf-style format string
     * @param ... Format arguments
     */
    static void infoF(const char* format, ...) {
        if (Config::Logging::LOG_LEVEL >= Config::Logging::INFO) {
            char timestamp[20];
            formatTimestamp(timestamp, sizeof(timestamp));

            Serial.printf("%s [INFO]  ", timestamp);

            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            Serial.println(buffer);
        }
    }

    /**
     * @brief Log error message
     * @param message Message to log
     */
    static void error(const char* message) {
        if (Config::Logging::LOG_LEVEL >= Config::Logging::ERROR) {
            char timestamp[20];
            formatTimestamp(timestamp, sizeof(timestamp));
            Serial.printf("%s [ERROR] %s\n", timestamp, message);
        }
    }

    /**
     * @brief Log error message with formatting
     * @param format Printf-style format string
     * @param ... Format arguments
     */
    static void errorF(const char* format, ...) {
        if (Config::Logging::LOG_LEVEL >= Config::Logging::ERROR) {
            char timestamp[20];
            formatTimestamp(timestamp, sizeof(timestamp));

            Serial.printf("%s [ERROR] ", timestamp);

            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            Serial.println(buffer);
        }
    }

    /**
     * @brief Log raw message without timestamp or level
     * @param message Message to log
     */
    static void raw(const char* message) {
        Serial.println(message);
    }

    /**
     * @brief Print separator line
     */
    static void separator() {
        Serial.println("========================================");
    }

    /**
     * @brief Print header with message
     * @param message Header message
     */
    static void header(const char* message) {
        separator();
        Serial.println(message);
        separator();
    }
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::debug(msg)
#define LOG_DEBUG_F(fmt, ...) Logger::debugF(fmt, ##__VA_ARGS__)

#define LOG_INFO(msg) Logger::info(msg)
#define LOG_INFO_F(fmt, ...) Logger::infoF(fmt, ##__VA_ARGS__)

#define LOG_ERROR(msg) Logger::error(msg)
#define LOG_ERROR_F(fmt, ...) Logger::errorF(fmt, ##__VA_ARGS__)

#define LOG_RAW(msg) Logger::raw(msg)
#define LOG_SEPARATOR() Logger::separator()
#define LOG_HEADER(msg) Logger::header(msg)
