#pragma once
#include <WebServer.h>
#include "BLEKeyboardManager.h"
#include "LEDManager.h"
#include "auth/authenticator.h"
#include "utils/rate_limiter.h"
#include "utils/validation.h"
#include "utils/logger.h"
#include "config.h"

/**
 * @class WebServerManager
 * @brief Manages HTTP web server and endpoints
 *
 * This class encapsulates the HTTP server with:
 * - Endpoint routing
 * - Authentication checks
 * - Rate limiting
 * - Request handling
 * - Dependency injection for BLE and LED managers
 *
 * Example usage:
 *   WebServerManager webServer(80);
 *   webServer.begin(&bleManager, &ledManager, &auth);
 *
 *   // In loop:
 *   webServer.handleClient();
 */
class WebServerManager {
private:
    WebServer server;
    BLEKeyboardManager* bleManager;
    LEDManager* ledManager;
    Authenticator* authenticator;
    RateLimiter rateLimiter;

    /**
     * @brief Root endpoint - returns API help
     */
    void handleRoot() {
        const char help[] PROGMEM =
            "ESP32 BLE Keyboard Remote - Secure Edition v2.0\n\n"
            "Available endpoints:\n"
            "  POST /ctrlaltdel      - Send Ctrl+Alt+Del\n"
            "  POST /sleep           - Send Win+X, U, S (Sleep)\n"
            "  POST /led/toggle      - Toggle LED\n"
            "  POST /type?msg=TEXT   - Type text via BLE keyboard\n"
            "  GET  /status          - Get system status\n"
            "  GET  /                - Show this help\n\n"
            "Authentication:\n"
            "  All endpoints (except / and /status) require X-API-Key header\n\n"
            "Rate Limiting:\n"
            "  Maximum 5 requests per second per IP\n\n"
            "Security:\n"
            "  - Authentication required\n"
            "  - Input validation enforced\n"
            "  - Rate limiting active\n"
            "  - Maximum message length: 1000 characters\n\n"
            "Architecture:\n"
            "  - Modular design with manager classes\n"
            "  - Non-blocking operations\n"
            "  - Overflow-safe timing\n"
            "  - Watchdog protection\n";

        server.send(200, "text/plain", help);
    }

    /**
     * @brief Status endpoint - returns system status (no auth required)
     */
    void handleStatus() {
        char json[512];
        snprintf(json, sizeof(json),
            "{"
            "\"ble\":{\"connected\":%s,\"busy\":%s,\"progress\":%d},"
            "\"led\":{\"state\":%s,\"flashing\":%s},"
            "\"uptime\":%lu,"
            "\"rateLimit\":{\"tracked\":%d}"
            "}",
            bleManager->isConnected() ? "true" : "false",
            bleManager->isBusy() ? "true" : "false",
            bleManager->getSendProgress(),
            ledManager->getManualState() ? "true" : "false",
            ledManager->isFlashing() ? "true" : "false",
            millis() / 1000,
            rateLimiter.getTrackedClientCount()
        );

        server.send(200, "application/json", json);
    }

    /**
     * @brief Handle Ctrl+Alt+Del request
     */
    void handleCtrlAlt() {
        // Authentication
        if (!authenticator->authenticate(server)) {
            authenticator->sendUnauthorized(server);
            return;
        }

        // Rate limiting
        if (!rateLimiter.checkLimit(server.client().remoteIP())) {
            Authenticator::sendError(server, ErrorCode::RATE_LIMIT_EXCEEDED);
            return;
        }

        LOG_INFO("Ctrl+Alt+Del requested");

        // Execute command
        ErrorCode result = bleManager->sendCtrlAltDel();

        if (result == ErrorCode::SUCCESS) {
            Authenticator::sendSuccess(server, "Sent Ctrl+Alt+Del");
        } else {
            Authenticator::sendError(server, result);
        }
    }

    /**
     * @brief Handle sleep command request
     */
    void handleSleep() {
        // Authentication
        if (!authenticator->authenticate(server)) {
            authenticator->sendUnauthorized(server);
            return;
        }

        // Rate limiting
        if (!rateLimiter.checkLimit(server.client().remoteIP())) {
            Authenticator::sendError(server, ErrorCode::RATE_LIMIT_EXCEEDED);
            return;
        }

        LOG_INFO("Sleep command requested");

        // Execute command
        ErrorCode result = bleManager->sendSleepCombo();

        if (result == ErrorCode::SUCCESS) {
            Authenticator::sendSuccess(server, "Sent Sleep Combo");
        } else {
            Authenticator::sendError(server, result);
        }
    }

    /**
     * @brief Handle LED toggle request
     */
    void handleLedToggle() {
        // Authentication
        if (!authenticator->authenticate(server)) {
            authenticator->sendUnauthorized(server);
            return;
        }

        // Rate limiting
        if (!rateLimiter.checkLimit(server.client().remoteIP())) {
            Authenticator::sendError(server, ErrorCode::RATE_LIMIT_EXCEEDED);
            return;
        }

        // Toggle LED
        bool newState = ledManager->toggle();

        char msg[64];
        snprintf(msg, sizeof(msg), "LED is now %s", newState ? "ON" : "OFF");
        LOG_INFO_F("LED toggled: %s", newState ? "ON" : "OFF");

        Authenticator::sendSuccess(server, msg);
    }

    /**
     * @brief Handle text typing request
     */
    void handleType() {
        // Authentication
        if (!authenticator->authenticate(server)) {
            authenticator->sendUnauthorized(server);
            return;
        }

        // Rate limiting
        if (!rateLimiter.checkLimit(server.client().remoteIP())) {
            Authenticator::sendError(server, ErrorCode::RATE_LIMIT_EXCEEDED);
            return;
        }

        // Validate parameter presence
        if (!server.hasArg("msg")) {
            Authenticator::sendError(server, ErrorCode::INVALID_PARAMETER);
            return;
        }

        // Get message and convert to char array
        String msgString = server.arg("msg");
        char msg[Config::BLE::MAX_MESSAGE_LENGTH + 1];
        msgString.toCharArray(msg, sizeof(msg));

        // Validate message
        auto validationResult = Validation::validateMessage(msgString);
        if (!validationResult.valid) {
            Authenticator::sendError(server, validationResult.errorCode);
            return;
        }

        // Log sanitized message
        LOG_INFO_F("Typing: %s",
                   Validation::sanitizeForLog(msgString, 50).c_str());

        // Queue message for non-blocking send
        ErrorCode result = bleManager->queueText(msg);

        if (result == ErrorCode::SUCCESS) {
            // Return accepted status immediately
            char response[128];
            snprintf(response, sizeof(response),
                "{\"status\":\"accepted\","
                "\"message\":\"Message queued for sending\","
                "\"length\":%d}",
                strlen(msg)
            );
            server.send(202, "application/json", response);
        } else {
            Authenticator::sendError(server, result);
        }
    }

    /**
     * @brief Register all HTTP routes
     */
    void registerRoutes() {
        server.on("/", HTTP_GET, [this]() { handleRoot(); });
        server.on("/status", HTTP_GET, [this]() { handleStatus(); });
        server.on("/ctrlaltdel", HTTP_POST, [this]() { handleCtrlAlt(); });
        server.on("/sleep", HTTP_POST, [this]() { handleSleep(); });
        server.on("/led/toggle", HTTP_POST, [this]() { handleLedToggle(); });
        server.on("/type", HTTP_POST, [this]() { handleType(); });
    }

public:
    /**
     * @brief Construct web server manager
     * @param port HTTP port (default: 80)
     */
    explicit WebServerManager(uint16_t port = Config::HTTP::SERVER_PORT)
        : server(port), bleManager(nullptr), ledManager(nullptr),
          authenticator(nullptr) {}

    /**
     * @brief Initialize web server with dependencies
     * @param ble BLE keyboard manager instance
     * @param led LED manager instance
     * @param auth Authenticator instance
     */
    void begin(BLEKeyboardManager* ble, LEDManager* led, Authenticator* auth) {
        bleManager = ble;
        ledManager = led;
        authenticator = auth;

        registerRoutes();
        server.begin();

        LOG_INFO("HTTP server started");
    }

    /**
     * @brief Handle incoming HTTP requests (non-blocking)
     *
     * Call this in loop()
     */
    void handleClient() {
        server.handleClient();

        // Periodic rate limiter cleanup
        static unsigned long lastCleanup = 0;
        if (TimeUtils::hasElapsed(lastCleanup, 60000)) {
            rateLimiter.cleanup();
            lastCleanup = millis();
        }
    }

    /**
     * @brief Get server port
     * @return HTTP port number
     */
    uint16_t getPort() const {
        return Config::HTTP::SERVER_PORT;
    }

    /**
     * @brief Get rate limiter statistics
     * @return Number of tracked clients
     */
    size_t getTrackedClients() const {
        return rateLimiter.getTrackedClientCount();
    }
};
