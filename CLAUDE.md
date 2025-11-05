# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-based BLE keyboard remote control built with PlatformIO and the Arduino framework. It provides both Bluetooth Low Energy keyboard functionality and an HTTP web server interface for remote control of a computer.

## Development Environment

### Required Setup

1. **Secrets File**: Create `src/secrets.h` with WiFi credentials before building:
```cpp
#ifndef SECRETS_H
#define SECRETS_H
#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourPassword"
#endif
```
This file is gitignored and must never be committed.

### Build Commands

```bash
# Build the project
pio run

# Upload to ESP32 board
pio run --target upload

# Open serial monitor (115200 baud)
pio device monitor

# Build and upload in one command
pio run --target upload && pio device monitor

# Clean build files
pio run --target clean
```

## Architecture

### Core Components

The system operates on a single-threaded event loop architecture with three main subsystems:

1. **BLE Keyboard Service** (src/main.cpp:7-8)
   - Initialized as `BleKeyboard bleKeyboard("TopoConKeyboard", "Topo Consulting LLC", 100)`
   - Handles keyboard command transmission via Bluetooth
   - Must check `bleKeyboard.isConnected()` before sending commands
   - All BLE functions are in the "BLE Functions" section

2. **HTTP Web Server** (src/main.cpp:11, 103-165)
   - Runs on port 80
   - Routes registered in setup(): /, /ctrlaltdel, /sleep, /led/toggle, /type
   - All handlers follow pattern: perform action, send response
   - `/type` endpoint sends text in 4-character chunks with 100ms delays for reliability

3. **WiFi Monitoring System** (src/main.cpp:63-101)
   - Runs in loop() via `checkWiFiConnection()`
   - Tracks connection state and disconnection duration
   - LED flash behavior: starts flashing (5s on/5s off) after 60 seconds of disconnection
   - Automatically attempts reconnection in background

### Key Implementation Details

- **Text Typing**: The `/type` handler (src/main.cpp:137-165) chunks messages into 4-character segments to avoid overwhelming the BLE connection. Adjust `MAX_CHUNK` if reliability issues occur.

- **WiFi Recovery**: The system uses a non-blocking reconnection strategy. If initial connection fails (60 second timeout), WiFi.reconnect() enables background reconnection while the system continues operating.

- **LED Control**: Pin 12 (GPIO12) serves dual purpose - manual toggle via `/led/toggle` and automatic WiFi status indication. The flashing behavior overrides manual state when WiFi is disconnected >60s.

### PlatformIO Configuration

- Platform: espressif32
- Board: esp32dev
- Framework: arduino
- Monitor speed: 115200 baud
- Partition scheme: huge_app.csv (for larger BLE applications)
- Library dependency: ESP32 BLE Keyboard v0.3.2

## Common Patterns

When adding new HTTP endpoints:
1. Create handler function in "HTTP Handlers" section
2. Register route in setup() with `server.on("/path", handlerFunction)`
3. Always validate BLE connection state before keyboard operations
4. Return appropriate HTTP status codes (200 for success, 400 for errors)

When adding new BLE keyboard commands:
1. Check `bleKeyboard.isConnected()` first
2. Use `bleKeyboard.press()` for modifier keys and key combinations
3. Call `bleKeyboard.releaseAll()` after command sequence
4. Add appropriate delays between key sequences (typically 100-500ms)

## Serial Monitor Output

The device IP address is printed to serial during startup. This is required to access HTTP endpoints. Monitor at 115200 baud to view:
- WiFi connection status
- Device IP address
- BLE connection state
- Command execution logs
