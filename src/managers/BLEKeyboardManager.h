#pragma once
#include <BleKeyboard.h>
#include "config.h"
#include "error_codes.h"
#include "utils/time_utils.h"

/**
 * @class BLEKeyboardManager
 * @brief Manages BLE keyboard connection and command sending
 *
 * This class encapsulates all BLE keyboard operations including:
 * - Connection management
 * - Non-blocking text sending via queue
 * - Special key combinations (Ctrl+Alt+Del, Sleep)
 *
 * Example usage:
 *   BLEKeyboardManager bleManager;
 *   bleManager.begin();
 *
 *   // In loop:
 *   bleManager.update();
 *
 *   // Send commands:
 *   if (bleManager.isConnected()) {
 *     bleManager.sendCtrlAltDel();
 *     bleManager.queueText("Hello World");
 *   }
 */
class BLEKeyboardManager {
private:
    BleKeyboard keyboard;

    // Non-blocking send queue
    struct SendQueue {
        char buffer[Config::BLE::MAX_MESSAGE_LENGTH + 1];
        size_t position;
        unsigned long lastSendTime;
        bool active;

        void reset() {
            buffer[0] = '\0';
            position = 0;
            lastSendTime = 0;
            active = false;
        }

        void start(const char* text) {
            strncpy(buffer, text, Config::BLE::MAX_MESSAGE_LENGTH);
            buffer[Config::BLE::MAX_MESSAGE_LENGTH] = '\0';
            position = 0;
            lastSendTime = millis();
            active = true;
        }

        size_t getLength() const {
            return strlen(buffer);
        }
    } sendQueue;

    /**
     * @brief Process send queue (called by update())
     */
    void processSendQueue() {
        if (!sendQueue.active) return;

        // Check if enough time has passed since last chunk
        if (!TimeUtils::hasElapsed(sendQueue.lastSendTime, Config::BLE::CHUNK_DELAY_MS)) {
            return;
        }

        // Verify BLE is still connected
        if (!keyboard.isConnected()) {
            sendQueue.reset();
            return;
        }

        // Calculate chunk to send
        size_t totalLength = sendQueue.getLength();
        size_t remaining = totalLength - sendQueue.position;
        size_t chunkLen = (remaining < Config::BLE::TEXT_CHUNK_SIZE)
            ? remaining
            : Config::BLE::TEXT_CHUNK_SIZE;

        // Create chunk
        char chunk[Config::BLE::TEXT_CHUNK_SIZE + 1];
        strncpy(chunk, sendQueue.buffer + sendQueue.position, chunkLen);
        chunk[chunkLen] = '\0';

        // Send chunk
        if (keyboard.print(chunk)) {
            sendQueue.position += chunkLen;
            sendQueue.lastSendTime = millis();

            // Check if done
            if (sendQueue.position >= totalLength) {
                keyboard.releaseAll();
                sendQueue.reset();
            }
        } else {
            // Send failed
            sendQueue.reset();
        }
    }

public:
    /**
     * @brief Construct BLE keyboard manager
     */
    BLEKeyboardManager()
        : keyboard(
            Config::BLE::DEVICE_NAME,
            Config::BLE::MANUFACTURER,
            Config::BLE::BATTERY_LEVEL
        ) {
        sendQueue.reset();
    }

    /**
     * @brief Initialize BLE keyboard
     *
     * Call this in setup()
     */
    void begin() {
        keyboard.begin();
    }

    /**
     * @brief Update manager state (non-blocking)
     *
     * Call this in loop() to process send queue
     */
    void update() {
        processSendQueue();
    }

    /**
     * @brief Check if BLE keyboard is connected
     * @return true if connected to a device
     */
    bool isConnected() const {
        return keyboard.isConnected();
    }

    /**
     * @brief Check if currently sending text
     * @return true if send queue is active
     */
    bool isBusy() const {
        return sendQueue.active;
    }

    /**
     * @brief Get current send progress
     * @return Percentage complete (0-100), or 0 if not sending
     */
    uint8_t getSendProgress() const {
        if (!sendQueue.active) return 0;

        size_t total = sendQueue.getLength();
        if (total == 0) return 100;

        return (sendQueue.position * 100) / total;
    }

    /**
     * @brief Send Ctrl+Alt+Del key combination
     * @return Error code
     */
    ErrorCode sendCtrlAltDel() {
        if (!keyboard.isConnected()) {
            return ErrorCode::BLE_NOT_CONNECTED;
        }

        keyboard.press(KEY_LEFT_CTRL);
        keyboard.press(KEY_LEFT_ALT);
        keyboard.press(KEY_DELETE);
        delay(Config::BLE::KEY_PRESS_DURATION_MS);
        keyboard.releaseAll();

        return ErrorCode::SUCCESS;
    }

    /**
     * @brief Send Windows sleep command sequence (Win+X, U, S)
     * @return Error code
     */
    ErrorCode sendSleepCombo() {
        if (!keyboard.isConnected()) {
            return ErrorCode::BLE_NOT_CONNECTED;
        }

        // Win+X
        keyboard.press(KEY_LEFT_GUI);
        keyboard.press('x');
        keyboard.releaseAll();
        delay(Config::BLE::SLEEP_COMBO_DELAY_MS);

        // U
        keyboard.press('u');
        keyboard.releaseAll();
        delay(Config::BLE::SLEEP_COMBO_DELAY_MS);

        // S
        keyboard.press('s');
        keyboard.releaseAll();

        return ErrorCode::SUCCESS;
    }

    /**
     * @brief Queue text for non-blocking transmission
     * @param text Text to send (max 1000 characters)
     * @return Error code
     */
    ErrorCode queueText(const char* text) {
        if (!keyboard.isConnected()) {
            return ErrorCode::BLE_NOT_CONNECTED;
        }

        if (sendQueue.active) {
            return ErrorCode::BUSY;
        }

        if (text == nullptr || text[0] == '\0') {
            return ErrorCode::MESSAGE_EMPTY;
        }

        size_t len = strlen(text);
        if (len > Config::BLE::MAX_MESSAGE_LENGTH) {
            return ErrorCode::MESSAGE_TOO_LONG;
        }

        sendQueue.start(text);
        return ErrorCode::SUCCESS;
    }

    /**
     * @brief Get BLE device name
     * @return Device name string
     */
    const char* getDeviceName() const {
        return Config::BLE::DEVICE_NAME;
    }
};
