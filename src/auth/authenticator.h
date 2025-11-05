#pragma once
#include <WebServer.h>
#include "error_codes.h"

/**
 * @file authenticator.h
 * @brief API key authentication for HTTP endpoints
 *
 * Provides simple API key authentication via X-API-Key header.
 *
 * Usage:
 *   Authenticator auth(API_KEY);
 *   if (!auth.authenticate(server)) {
 *     auth.sendUnauthorized(server);
 *     return;
 *   }
 */

class Authenticator {
private:
    const char* apiKey;

public:
    /**
     * @brief Construct authenticator with API key
     * @param key API key to validate against
     */
    explicit Authenticator(const char* key) : apiKey(key) {}

    /**
     * @brief Authenticate incoming request
     * @param server WebServer instance
     * @return true if request has valid API key
     */
    bool authenticate(WebServer& server) {
        if (!server.hasHeader("X-API-Key")) {
            return false;
        }

        String providedKey = server.header("X-API-Key");
        return providedKey.equals(apiKey);
    }

    /**
     * @brief Send unauthorized response
     * @param server WebServer instance
     */
    void sendUnauthorized(WebServer& server) {
        server.send(
            httpStatusCode(ErrorCode::UNAUTHORIZED),
            "application/json",
            "{\"error\":\"Unauthorized\",\"message\":\"Valid API key required in X-API-Key header\"}"
        );
    }

    /**
     * @brief Send error response
     * @param server WebServer instance
     * @param code Error code
     */
    static void sendError(WebServer& server, ErrorCode code) {
        char json[256];
        snprintf(json, sizeof(json),
            "{\"error\":\"%s\",\"code\":%d}",
            errorMessage(code),
            static_cast<int>(code)
        );
        server.send(httpStatusCode(code), "application/json", json);
    }

    /**
     * @brief Send success response
     * @param server WebServer instance
     * @param message Success message
     */
    static void sendSuccess(WebServer& server, const char* message) {
        char json[256];
        snprintf(json, sizeof(json),
            "{\"status\":\"success\",\"message\":\"%s\"}",
            message
        );
        server.send(200, "application/json", json);
    }
};
