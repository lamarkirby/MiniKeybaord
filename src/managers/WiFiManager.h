#pragma once
#include <WiFi.h>
#include "config.h"
#include "utils/time_utils.h"

/**
 * @class WiFiManager
 * @brief Manages WiFi connection and monitoring
 *
 * This class encapsulates all WiFi operations including:
 * - Connection establishment
 * - Automatic reconnection
 * - Connection monitoring
 * - Status reporting
 *
 * Example usage:
 *   WiFiManager wifiManager;
 *   wifiManager.begin("MySSID", "MyPassword");
 *
 *   // In loop:
 *   wifiManager.update();
 *
 *   if (wifiManager.isConnected()) {
 *     IPAddress ip = wifiManager.getIP();
 *   }
 */
class WiFiManager {
private:
    struct State {
        bool connected;
        unsigned long disconnectTime;
        unsigned long lastStatusCheck;
        bool hasBeenConnected;  // Track if we've ever connected

        State() : connected(false), disconnectTime(0),
                  lastStatusCheck(0), hasBeenConnected(false) {}
    } state;

    char ssid[64];
    char password[64];

public:
    /**
     * @brief Construct WiFi manager
     */
    WiFiManager() {
        ssid[0] = '\0';
        password[0] = '\0';
    }

    /**
     * @brief Initialize WiFi connection
     * @param wifiSsid Network SSID
     * @param wifiPassword Network password
     * @param timeoutMs Connection timeout in milliseconds (default: 60000)
     * @return true if connected successfully
     */
    bool begin(const char* wifiSsid, const char* wifiPassword,
               uint32_t timeoutMs = Config::WiFi::CONNECT_TIMEOUT_MS) {

        // Store credentials
        strncpy(ssid, wifiSsid, sizeof(ssid) - 1);
        ssid[sizeof(ssid) - 1] = '\0';

        strncpy(password, wifiPassword, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';

        // Start connection
        WiFi.begin(ssid, password);

        // Wait for connection with timeout
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED &&
               !TimeUtils::hasElapsed(startTime, timeoutMs)) {
            delay(100);
        }

        if (WiFi.status() == WL_CONNECTED) {
            state.connected = true;
            state.hasBeenConnected = true;
            return true;
        } else {
            state.disconnectTime = millis();
            WiFi.reconnect();  // Enable background reconnection
            return false;
        }
    }

    /**
     * @brief Update WiFi state (non-blocking)
     *
     * Call this in loop() to monitor connection status.
     * Throttled to check once per second for efficiency.
     */
    void update() {
        // Throttle status checks
        if (!TimeUtils::hasElapsed(state.lastStatusCheck,
                                   Config::WiFi::STATUS_CHECK_INTERVAL_MS)) {
            return;
        }

        state.lastStatusCheck = millis();

        // Check connection status
        if (WiFi.status() == WL_CONNECTED) {
            if (!state.connected) {
                // Just reconnected
                state.connected = true;
                state.hasBeenConnected = true;
                state.disconnectTime = 0;
            }
        } else {
            if (state.connected) {
                // Just disconnected
                state.connected = false;
                state.disconnectTime = millis();
            }
        }
    }

    /**
     * @brief Check if WiFi is currently connected
     * @return true if connected
     */
    bool isConnected() const {
        return state.connected;
    }

    /**
     * @brief Check if WiFi has been disconnected for a long time
     * @return true if disconnected for more than configured alert time
     */
    bool isDisconnectedLongTerm() const {
        if (state.connected) return false;
        if (state.disconnectTime == 0) return false;

        return TimeUtils::hasElapsed(state.disconnectTime,
                                     Config::WiFi::DISCONNECT_ALERT_MS);
    }

    /**
     * @brief Get current IP address
     * @return IPAddress object (0.0.0.0 if not connected)
     */
    IPAddress getIP() const {
        return WiFi.localIP();
    }

    /**
     * @brief Get WiFi signal strength
     * @return RSSI in dBm (typically -100 to 0)
     */
    int32_t getRSSI() const {
        return WiFi.RSSI();
    }

    /**
     * @brief Get WiFi SSID
     * @return SSID string
     */
    const char* getSSID() const {
        return ssid;
    }

    /**
     * @brief Get connection status string
     * @return Human-readable status
     */
    const char* getStatusString() const {
        if (state.connected) {
            return "Connected";
        } else if (state.hasBeenConnected) {
            return "Disconnected (reconnecting...)";
        } else {
            return "Never connected";
        }
    }

    /**
     * @brief Get disconnect duration in seconds
     * @return Seconds since disconnect, or 0 if connected
     */
    uint32_t getDisconnectDuration() const {
        if (state.connected || state.disconnectTime == 0) {
            return 0;
        }
        return TimeUtils::timeDiff(state.disconnectTime) / 1000;
    }

    /**
     * @brief Force reconnection attempt
     */
    void reconnect() {
        if (!state.connected) {
            WiFi.reconnect();
        }
    }

    /**
     * @brief Disconnect from WiFi
     */
    void disconnect() {
        WiFi.disconnect();
        state.connected = false;
    }
};
