#include <Arduino.h>
#include <esp_task_wdt.h>
#include "secrets.h"
#include "config.h"
#include "managers/BLEKeyboardManager.h"
#include "managers/WiFiManager.h"
#include "managers/LEDManager.h"
#include "managers/WebServerManager.h"
#include "auth/authenticator.h"
#include "utils/logger.h"

/**
 * @file main.cpp
 * @brief ESP32 BLE Keyboard - Main Application
 *
 * Architecture:
 * - Modular design with manager classes
 * - Non-blocking operations throughout
 * - Dependency injection for testability
 * - Clean separation of concerns
 *
 * Flow:
 * 1. setup() initializes all managers
 * 2. loop() calls update() on each manager
 * 3. Managers handle their own state and timing
 */

// ===== Global Manager Instances =====
BLEKeyboardManager bleManager;
WiFiManager wifiManager;
LEDManager ledManager(Config::LED::PIN);
WebServerManager webServer(Config::HTTP::SERVER_PORT);
Authenticator authenticator(API_KEY);

// ====== Setup ======

void setup() {
    Serial.begin(115200);
    delay(100);  // Allow serial to stabilize

    LOG_HEADER("ESP32 BLE Keyboard - Secure Edition v2.0");
    LOG_RAW("");

    // Initialize watchdog timer
    esp_task_wdt_init(Config::Watchdog::TIMEOUT_SECONDS, true);
    esp_task_wdt_add(NULL);
    LOG_INFO_F("Watchdog timer enabled (%d seconds)", Config::Watchdog::TIMEOUT_SECONDS);

    // Initialize LED
    ledManager.begin();
    LOG_INFO("LED manager initialized");

    // Initialize BLE keyboard
    bleManager.begin();
    LOG_INFO_F("BLE keyboard started: %s", bleManager.getDeviceName());

    // Connect to WiFi
    LOG_INFO_F("Connecting to WiFi: %s", WIFI_SSID);
    bool wifiConnected = wifiManager.begin(WIFI_SSID, WIFI_PASSWORD);

    if (wifiConnected) {
        LOG_INFO_F("WiFi connected! IP: %s", wifiManager.getIP().toString().c_str());
    } else {
        LOG_ERROR("WiFi connection timeout. Will retry in background.");
    }

    // Initialize web server with dependencies
    webServer.begin(&bleManager, &ledManager, &authenticator);

    // Print startup summary
    LOG_RAW("");
    LOG_SEPARATOR();
    LOG_RAW("System Ready");
    LOG_SEPARATOR();
    LOG_INFO_F("BLE Device: %s", bleManager.getDeviceName());
    LOG_INFO_F("WiFi Status: %s", wifiManager.getStatusString());
    if (wifiManager.isConnected()) {
        LOG_INFO_F("IP Address: %s", wifiManager.getIP().toString().c_str());
        LOG_INFO_F("Signal: %d dBm", wifiManager.getRSSI());
    }
    LOG_INFO_F("HTTP Port: %d", webServer.getPort());
    LOG_INFO_F("Rate Limit: %d req/%dms",
               Config::RateLimit::MAX_REQUESTS,
               Config::RateLimit::WINDOW_MS);
    LOG_RAW("");
    LOG_RAW("⚠️  SECURITY: API key required in X-API-Key header");
    LOG_SEPARATOR();
    LOG_RAW("");
}

// ====== Loop ======

void loop() {
    // Reset watchdog timer
    esp_task_wdt_reset();

    // Update all managers (all non-blocking)
    wifiManager.update();
    bleManager.update();
    ledManager.update();
    webServer.handleClient();

    // Update LED status based on WiFi state
    static bool lastWiFiState = false;
    bool currentWiFiState = wifiManager.isConnected();

    if (lastWiFiState != currentWiFiState) {
        // WiFi state changed
        if (currentWiFiState) {
            LOG_INFO_F("WiFi reconnected! IP: %s", wifiManager.getIP().toString().c_str());
            ledManager.setFlashing(false);
        } else {
            LOG_ERROR("WiFi disconnected!");
        }
        lastWiFiState = currentWiFiState;
    }

    // Check for long-term WiFi disconnect
    if (wifiManager.isDisconnectedLongTerm()) {
        ledManager.setFlashing(true);
    }
}
