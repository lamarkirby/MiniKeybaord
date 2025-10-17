# ESP32 BLE Keyboard Remote

## Overview

This project transforms an ESP32 microcontroller into a versatile BLE (Bluetooth Low Energy) keyboard remote control. It communicates with your computer via Bluetooth to send keyboard commands and text, while also providing a web-based HTTP interface for triggering actions.

## Features

- **BLE Keyboard Control** - Send Ctrl+Alt+Del, Windows sleep commands, and custom text via Bluetooth
- **WiFi Web Server** - Control the device remotely via HTTP endpoints
- **WiFi Monitoring** - LED flashing alerts when WiFi connection is lost for more than 60 seconds
- **Automatic Reconnection** - Device automatically attempts to reconnect to WiFi if the connection drops
- **LED Status Indicator** - Visual feedback for WiFi connection status

## Hardware Requirements

- ESP32 Development Board
- USB cable for programming and power
- LED (optional, for status indication on pin 12)

## Software Requirements

- PlatformIO (VS Code extension recommended)
- Arduino Framework for ESP32
- ESP32 BLE Keyboard library (v0.3.2)

## Installation

1. Clone or download this project
2. Open in PlatformIO
3. Create a `src/secrets.h` file with your WiFi credentials:

```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourPassword"

#endif
```

4. Add `secrets.h` to your `.gitignore` to prevent accidentally committing credentials
5. Upload to your ESP32 board

## API Endpoints

Access the device at its local IP address. Get the IP from the serial monitor during startup.

### Root Endpoint
```
GET /
```
Returns help information with all available endpoints.

### Send Ctrl+Alt+Del
```
GET /ctrlaltdel
```
Sends the Ctrl+Alt+Del key combination.

### Send Sleep Command
```
GET /sleep
```
Sends Win+X, U, S sequence to put Windows to sleep.

### Toggle LED
```
GET /led/toggle
```
Toggles the onboard LED on pin 12.

<img src="https://m.media-amazon.com/images/I/817gqxoGDGL._AC_SX679_.jpg" alt="SP32 Dev Board" width="30%"/>

### Type Text
```
GET /type?msg="Your text here"
```
Types the specified text via the BLE keyboard. Text is sent in 4-character chunks with 100ms delays between chunks for reliability.

## Configuration

- **LED_PIN**: Set to 12 (GPIO12) - change if using a different pin
- **WiFi Timeout**: 60 seconds for initial connection attempt
- **Disconnect Alert**: LED flashes after 60 seconds without WiFi
- **Text Chunk Size**: 4 characters per chunk (adjust `MAX_CHUNK` in code if needed)

## Serial Monitor

Connect to the serial monitor at **115200 baud** to view debug output and the device's IP address.

## Status Indicators

- **LED Off**: WiFi connected
- **LED On/Off Flashing (5s/5s)**: WiFi disconnected for more than 60 seconds
- **LED Stops Flashing**: WiFi connection restored

## Troubleshooting

**WiFi won't connect:**
- Verify SSID and password in `secrets.h`
- Check that your 2.4GHz WiFi network is accessible
- Device will attempt reconnection in the background

**BLE keyboard not working:**
- Ensure device is paired to your computer first
- Check the serial monitor to see if BLE is connected
- Call `/ctrlaltdel` or `/sleep` endpoints to verify

**Text not typing correctly:**
- Verify the text is URL-encoded properly if using special characters
- Use quotes around the message: `/type?msg="Hello World"`

## Security Notes

- WiFi credentials are stored in `secrets.h` which should never be committed to version control
- This device assumes a trusted network - add authentication if deploying on public networks
- Keep `secrets.h` safe and do not share it

## Future Improvements

- Input validation with message length limits
- URL encoding/decoding for special characters
- Status endpoint to check WiFi and BLE connection state
- PWM LED brightness control
- Request logging with timestamps
