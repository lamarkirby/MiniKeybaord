#pragma once
#include <Arduino.h>
#include "config.h"
#include "utils/time_utils.h"

/**
 * @class LEDManager
 * @brief Manages LED status indication
 *
 * This class encapsulates LED control with support for:
 * - Manual on/off toggle
 * - Automatic flashing (e.g., for WiFi disconnect alerts)
 * - Priority handling (flashing overrides manual state)
 *
 * Example usage:
 *   LEDManager ledManager(12);
 *   ledManager.begin();
 *
 *   // Manual control
 *   ledManager.toggle();
 *   ledManager.setManual(true);
 *
 *   // Automatic flashing
 *   ledManager.setFlashing(true);
 *
 *   // In loop:
 *   ledManager.update();
 */
class LEDManager {
private:
    uint8_t pin;
    bool manualState;
    bool flashingEnabled;
    bool flashState;
    unsigned long lastFlashToggle;

public:
    /**
     * @brief Construct LED manager
     * @param ledPin GPIO pin number for LED
     */
    explicit LEDManager(uint8_t ledPin = Config::LED::PIN)
        : pin(ledPin), manualState(false), flashingEnabled(false),
          flashState(false), lastFlashToggle(0) {}

    /**
     * @brief Initialize LED
     *
     * Call this in setup()
     */
    void begin() {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        manualState = false;
        flashingEnabled = false;
        flashState = false;
    }

    /**
     * @brief Update LED state (non-blocking)
     *
     * Call this in loop() to handle flashing
     */
    void update() {
        if (flashingEnabled) {
            // Handle automatic flashing
            if (TimeUtils::hasElapsed(lastFlashToggle, Config::LED::FLASH_INTERVAL_MS)) {
                flashState = !flashState;
                digitalWrite(pin, flashState ? HIGH : LOW);
                lastFlashToggle = millis();
            }
        } else {
            // Use manual state
            digitalWrite(pin, manualState ? HIGH : LOW);
        }
    }

    /**
     * @brief Toggle LED manual state
     * @return New state (true = on, false = off)
     *
     * Only affects LED when flashing is disabled
     */
    bool toggle() {
        manualState = !manualState;
        if (!flashingEnabled) {
            digitalWrite(pin, manualState ? HIGH : LOW);
        }
        return manualState;
    }

    /**
     * @brief Set LED manual state
     * @param state true for on, false for off
     *
     * Only affects LED when flashing is disabled
     */
    void setManual(bool state) {
        manualState = state;
        if (!flashingEnabled) {
            digitalWrite(pin, manualState ? HIGH : LOW);
        }
    }

    /**
     * @brief Enable or disable automatic flashing
     * @param enabled true to enable flashing, false to disable
     *
     * When flashing is enabled, it overrides manual state.
     * When disabled, manual state takes effect.
     */
    void setFlashing(bool enabled) {
        flashingEnabled = enabled;

        if (enabled) {
            // Start flashing - reset timing
            lastFlashToggle = millis();
            flashState = false;
        } else {
            // Restore manual state
            digitalWrite(pin, manualState ? HIGH : LOW);
        }
    }

    /**
     * @brief Get manual state
     * @return Current manual state (true = on, false = off)
     */
    bool getManualState() const {
        return manualState;
    }

    /**
     * @brief Check if flashing is enabled
     * @return true if flashing is active
     */
    bool isFlashing() const {
        return flashingEnabled;
    }

    /**
     * @brief Get current LED physical state
     * @return true if LED is currently lit
     */
    bool getCurrentState() const {
        return digitalRead(pin) == HIGH;
    }

    /**
     * @brief Get LED pin number
     * @return GPIO pin number
     */
    uint8_t getPin() const {
        return pin;
    }
};
