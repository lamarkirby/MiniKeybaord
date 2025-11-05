#pragma once
#include <Arduino.h>

/**
 * @file time_utils.h
 * @brief Overflow-safe timing utilities
 *
 * These utilities handle millis() overflow correctly.
 * The millis() function overflows after approximately 49 days,
 * wrapping from ~4,294,967,295 back to 0.
 */

namespace TimeUtils {
    /**
     * @brief Check if specified time interval has elapsed (overflow-safe)
     * @param start Start time from millis()
     * @param interval Interval to check in milliseconds
     * @return true if interval has elapsed since start
     *
     * This function handles millis() overflow correctly using unsigned
     * arithmetic properties. The subtraction wraps correctly even when
     * millis() overflows.
     *
     * Example:
     *   unsigned long start = millis();
     *   // ... some time passes ...
     *   if (TimeUtils::hasElapsed(start, 1000)) {
     *     // At least 1 second has passed
     *   }
     */
    inline bool hasElapsed(unsigned long start, unsigned long interval) {
        return (unsigned long)(millis() - start) >= interval;
    }

    /**
     * @brief Calculate time difference (overflow-safe)
     * @param start Start time
     * @param end End time (defaults to current time)
     * @return Time difference in milliseconds
     *
     * Example:
     *   unsigned long start = millis();
     *   delay(100);
     *   unsigned long elapsed = TimeUtils::timeDiff(start);
     *   // elapsed will be approximately 100
     */
    inline unsigned long timeDiff(unsigned long start, unsigned long end = 0) {
        if (end == 0) end = millis();
        return (unsigned long)(end - start);
    }

    /**
     * @brief Check if we're within a time window (overflow-safe)
     * @param timestamp Timestamp to check
     * @param windowMs Window size in milliseconds
     * @return true if current time is within windowMs of timestamp
     */
    inline bool withinWindow(unsigned long timestamp, unsigned long windowMs) {
        return timeDiff(timestamp) < windowMs;
    }
}
