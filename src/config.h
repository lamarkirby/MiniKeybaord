#pragma once
#include <Arduino.h>

/**
 * @file config.h
 * @brief Centralized configuration for ESP32 BLE Keyboard
 *
 * All magic numbers and configuration constants are defined here
 * to improve maintainability and make tuning easier.
 */

namespace Config {
    // BLE Keyboard Configuration
    namespace BLE {
        constexpr const char* DEVICE_NAME = "TopoConKeyboard";
        constexpr const char* MANUFACTURER = "Topo Consulting LLC";
        constexpr uint8_t BATTERY_LEVEL = 100;

        // Timing constants
        constexpr uint16_t KEY_PRESS_DURATION_MS = 100;
        constexpr uint16_t SLEEP_COMBO_DELAY_MS = 500;

        // Text sending configuration
        constexpr size_t TEXT_CHUNK_SIZE = 4;  // Characters per chunk for reliable BLE transmission
        constexpr uint16_t CHUNK_DELAY_MS = 100;  // Delay between chunks
        constexpr size_t MAX_MESSAGE_LENGTH = 1000;  // Maximum message length to prevent DoS
    }

    // WiFi Configuration
    namespace WiFi {
        constexpr uint32_t CONNECT_TIMEOUT_MS = 60000;  // 60 seconds
        constexpr uint32_t DISCONNECT_ALERT_MS = 60000;  // Alert after 60s without WiFi
        constexpr uint32_t STATUS_CHECK_INTERVAL_MS = 1000;  // Check WiFi status every second
    }

    // LED Configuration
    namespace LED {
        constexpr uint8_t PIN = 12;  // GPIO pin for status LED
        constexpr uint32_t FLASH_INTERVAL_MS = 5000;  // 5 seconds on/off when WiFi disconnected
    }

    // HTTP Server Configuration
    namespace HTTP {
        constexpr uint16_t SERVER_PORT = 80;
        constexpr uint32_t REQUEST_TIMEOUT_MS = 5000;
    }

    // Rate Limiting Configuration
    namespace RateLimit {
        constexpr uint32_t WINDOW_MS = 1000;  // Rate limit window (1 second)
        constexpr uint8_t MAX_REQUESTS = 5;  // Max requests per window
    }

    // Logging Configuration
    namespace Logging {
        enum Level {
            NONE = 0,
            ERROR = 1,
            INFO = 2,
            DEBUG = 3
        };

        #ifdef DEBUG
            constexpr Level LOG_LEVEL = DEBUG;
        #else
            constexpr Level LOG_LEVEL = INFO;
        #endif
    }

    // Watchdog Configuration
    namespace Watchdog {
        constexpr uint32_t TIMEOUT_SECONDS = 30;  // 30 second watchdog timeout
    }
}
