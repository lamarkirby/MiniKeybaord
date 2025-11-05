#pragma once
#include <map>
#include <IPAddress.h>
#include "config.h"
#include "time_utils.h"

/**
 * @file rate_limiter.h
 * @brief Rate limiting for HTTP requests
 *
 * Prevents abuse by limiting requests per IP address.
 *
 * Usage:
 *   RateLimiter limiter;
 *   if (!limiter.checkLimit(server.client().remoteIP())) {
 *     server.send(429, "text/plain", "Rate limit exceeded");
 *     return;
 *   }
 */

class RateLimiter {
private:
    struct ClientInfo {
        unsigned long lastRequestTime;
        uint8_t requestCount;
    };

    std::map<uint32_t, ClientInfo> clients;
    uint32_t windowMs;
    uint8_t maxRequests;
    unsigned long lastCleanup;

    /**
     * @brief Convert IPAddress to uint32_t for map key
     */
    static uint32_t ipToUint32(IPAddress ip) {
        return (uint32_t)ip;
    }

public:
    /**
     * @brief Construct rate limiter
     * @param window Rate limit window in milliseconds
     * @param max Maximum requests per window
     */
    RateLimiter(
        uint32_t window = Config::RateLimit::WINDOW_MS,
        uint8_t max = Config::RateLimit::MAX_REQUESTS
    ) : windowMs(window), maxRequests(max), lastCleanup(millis()) {}

    /**
     * @brief Check if request is within rate limit
     * @param ip Client IP address
     * @return true if request is allowed, false if rate limited
     */
    bool checkLimit(IPAddress ip) {
        uint32_t ipInt = ipToUint32(ip);
        unsigned long now = millis();

        // Find or create client entry
        auto it = clients.find(ipInt);

        if (it == clients.end()) {
            // New client
            clients[ipInt] = {now, 1};
            return true;
        }

        ClientInfo& info = it->second;

        // Check if we're outside the rate limit window
        if (TimeUtils::timeDiff(info.lastRequestTime, now) >= windowMs) {
            // Reset counter for new window
            info.lastRequestTime = now;
            info.requestCount = 1;
            return true;
        }

        // Within window - check count
        if (info.requestCount >= maxRequests) {
            return false;  // Rate limit exceeded
        }

        // Allow request and increment counter
        info.requestCount++;
        return true;
    }

    /**
     * @brief Cleanup old entries (call periodically)
     *
     * Removes entries for IPs that haven't made requests recently
     * to prevent unbounded memory growth.
     */
    void cleanup() {
        unsigned long now = millis();

        // Only cleanup every 10 windows
        if (!TimeUtils::hasElapsed(lastCleanup, windowMs * 10)) {
            return;
        }

        lastCleanup = now;

        // Remove entries older than 10 windows
        for (auto it = clients.begin(); it != clients.end();) {
            if (TimeUtils::timeDiff(it->second.lastRequestTime, now) > windowMs * 10) {
                it = clients.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Get current number of tracked clients
     * @return Number of clients being tracked
     */
    size_t getTrackedClientCount() const {
        return clients.size();
    }

    /**
     * @brief Clear all rate limit data
     */
    void reset() {
        clients.clear();
    }
};
