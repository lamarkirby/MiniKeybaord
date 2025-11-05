#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/**
 * @file Arduino.h
 * @brief Mock Arduino core for native testing
 *
 * This mock allows Arduino code to compile and run on native platform
 * for testing without ESP32 hardware.
 */

// Pin modes
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

// Pin states
#define LOW 0
#define HIGH 1

// Mock global state
extern unsigned long mock_millis_value;
extern uint8_t mock_pin_states[50];

// Arduino functions
inline unsigned long millis() { return mock_millis_value; }
inline void delay(unsigned long ms) { mock_millis_value += ms; }
inline void pinMode(uint8_t pin, uint8_t mode) { (void)pin; (void)mode; }
inline void digitalWrite(uint8_t pin, uint8_t value) {
    if (pin < 50) mock_pin_states[pin] = value;
}
inline uint8_t digitalRead(uint8_t pin) {
    return (pin < 50) ? mock_pin_states[pin] : 0;
}

// Serial mock
class MockSerial {
public:
    void begin(unsigned long baud) { (void)baud; }
    void println(const char* str) { printf("%s\n", str); }
    void print(const char* str) { printf("%s", str); }
    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
};

extern MockSerial Serial;

// String class (simplified)
class String {
private:
    char* buffer;
    size_t len;

public:
    String() : buffer(nullptr), len(0) {}

    String(const char* str) {
        len = strlen(str);
        buffer = new char[len + 1];
        strcpy(buffer, str);
    }

    String(const String& other) {
        len = other.len;
        buffer = new char[len + 1];
        strcpy(buffer, other.buffer ? other.buffer : "");
    }

    ~String() {
        delete[] buffer;
    }

    String& operator=(const String& other) {
        if (this != &other) {
            delete[] buffer;
            len = other.len;
            buffer = new char[len + 1];
            strcpy(buffer, other.buffer ? other.buffer : "");
        }
        return *this;
    }

    size_t length() const { return len; }
    char charAt(size_t index) const {
        return (index < len && buffer) ? buffer[index] : 0;
    }

    String substring(size_t start, size_t end) const {
        if (!buffer || start >= len) return String();
        size_t actualEnd = (end > len) ? len : end;
        size_t subLen = actualEnd - start;
        char* sub = new char[subLen + 1];
        strncpy(sub, buffer + start, subLen);
        sub[subLen] = '\0';
        String result(sub);
        delete[] sub;
        return result;
    }

    void toCharArray(char* buf, size_t bufsize) const {
        if (!buffer || !buf || bufsize == 0) return;
        strncpy(buf, buffer, bufsize - 1);
        buf[bufsize - 1] = '\0';
    }

    bool equals(const char* str) const {
        if (!buffer) return !str || str[0] == '\0';
        return strcmp(buffer, str) == 0;
    }

    const char* c_str() const { return buffer ? buffer : ""; }
};

// Helper for String comparison
inline bool operator==(const String& a, const String& b) {
    return strcmp(a.c_str(), b.c_str()) == 0;
}
