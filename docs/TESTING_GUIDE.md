# Testing Guide - Phase 1 Improvements

**Date:** 2025-11-05
**Phase:** 1 - Critical Fixes
**Status:** Ready for Testing

## Overview

This guide covers testing all Phase 1 improvements that were implemented:
- ✅ Authentication system
- ✅ Input validation
- ✅ Rate limiting
- ✅ Non-blocking BLE queue
- ✅ Overflow-safe timing
- ✅ Watchdog timer
- ✅ Error handling
- ✅ Configuration management

---

## Pre-Testing Setup

### 1. Update WiFi Credentials

Edit `src/secrets.h` and update your WiFi credentials:
```cpp
#define WIFI_SSID "YourActualNetworkName"
#define WIFI_PASSWORD "YourActualWiFiPassword"
```

### 2. Generate Secure API Key

Generate a secure random API key:
```bash
# On Mac/Linux:
openssl rand -hex 32

# Example output:
# 7f8a3e9d2c1b5a4f6e0d8c7b9a2e1f3d4c5b6a7e8f9d0c1b2a3e4f5d6c7b8a9
```

Update `src/secrets.h` with your generated key:
```cpp
#define API_KEY "your-generated-key-here"
```

### 3. Build and Upload

```bash
# Clean build
pio run --target clean

# Build
pio run

# Upload to ESP32
pio run --target upload

# Open serial monitor
pio device monitor
```

### 4. Note Your ESP32 IP Address

Watch the serial monitor output during startup. You'll see:
```
✓ WiFi connected!
IP Address: 192.168.1.XXX
```

Make note of this IP address - you'll need it for testing.

---

## Test Suite

### Test 1: Authentication System

#### Test 1.1: Request Without API Key (Should Fail)
```bash
curl -X POST http://192.168.1.XXX/ctrlaltdel
```

**Expected Response:**
```json
{
  "error": "Unauthorized",
  "message": "Valid API key required in X-API-Key header"
}
```
**HTTP Status:** 401 Unauthorized

**✅ PASS CRITERIA:** Request rejected with 401 status

---

#### Test 1.2: Request With Wrong API Key (Should Fail)
```bash
curl -X POST \
  -H "X-API-Key: wrong-key" \
  http://192.168.1.XXX/ctrlaltdel
```

**Expected Response:**
```json
{
  "error": "Unauthorized",
  "message": "Valid API key required in X-API-Key header"
}
```
**HTTP Status:** 401 Unauthorized

**✅ PASS CRITERIA:** Request rejected with 401 status

---

#### Test 1.3: Request With Correct API Key (Should Succeed)
```bash
curl -X POST \
  -H "X-API-Key: your-actual-api-key" \
  http://192.168.1.XXX/led/toggle
```

**Expected Response:**
```json
{
  "status": "success",
  "message": "LED is now ON"
}
```
**HTTP Status:** 200 OK

**✅ PASS CRITERIA:** Request succeeds, LED toggles

---

### Test 2: Input Validation

#### Test 2.1: Empty Message (Should Fail)
```bash
curl -X POST \
  -H "X-API-Key: your-api-key" \
  "http://192.168.1.XXX/type?msg="
```

**Expected Response:**
```json
{
  "error": "Message cannot be empty",
  "code": 5
}
```
**HTTP Status:** 400 Bad Request

**✅ PASS CRITERIA:** Request rejected with appropriate error

---

#### Test 2.2: Message Too Long (Should Fail)
```bash
# Generate 1001 character message
curl -X POST \
  -H "X-API-Key: your-api-key" \
  "http://192.168.1.XXX/type?msg=$(python3 -c 'print("A"*1001)')"
```

**Expected Response:**
```json
{
  "error": "Message exceeds maximum length",
  "code": 4
}
```
**HTTP Status:** 400 Bad Request

**✅ PASS CRITERIA:** Request rejected, system still responsive

---

#### Test 2.3: Invalid Characters (Should Fail)
```bash
curl -X POST \
  -H "X-API-Key: your-api-key" \
  "http://192.168.1.XXX/type?msg=$(printf '\x01\x02\x03')"
```

**Expected Response:**
```json
{
  "error": "Message contains invalid characters",
  "code": 7
}
```
**HTTP Status:** 400 Bad Request

**✅ PASS CRITERIA:** Request rejected with invalid characters error

---

#### Test 2.4: Valid Message (Should Succeed)
```bash
curl -X POST \
  -H "X-API-Key: your-api-key" \
  "http://192.168.1.XXX/type?msg=Hello%20World"
```

**Expected Response:**
```json
{
  "status": "accepted",
  "message": "Message queued for sending",
  "length": 11
}
```
**HTTP Status:** 202 Accepted

**Serial Monitor Should Show:**
```
Typing: Hello World
BLE send complete
```

**✅ PASS CRITERIA:** Message queued and sent via BLE

---

### Test 3: Rate Limiting

#### Test 3.1: Exceed Rate Limit
```bash
# Send 10 rapid requests (limit is 5 per second)
for i in {1..10}; do
  curl -X POST \
    -H "X-API-Key: your-api-key" \
    http://192.168.1.XXX/led/toggle &
done
wait
```

**Expected Behavior:**
- First 5 requests succeed (200 OK)
- Remaining 5 requests fail with 429

**Expected Response (after 5th request):**
```json
{
  "error": "Rate limit exceeded - too many requests",
  "code": 8
}
```
**HTTP Status:** 429 Too Many Requests

**✅ PASS CRITERIA:** Only 5 requests succeed per second

---

#### Test 3.2: Rate Limit Recovery
```bash
# Send request
curl -X POST \
  -H "X-API-Key: your-api-key" \
  http://192.168.1.XXX/led/toggle

# Wait 2 seconds
sleep 2

# Try again - should succeed
curl -X POST \
  -H "X-API-Key: your-api-key" \
  http://192.168.1.XXX/led/toggle
```

**✅ PASS CRITERIA:** Rate limit resets after window expires

---

### Test 4: Non-Blocking BLE Queue

#### Test 4.1: Long Message Doesn't Block System
```bash
# Terminal 1: Send long message (250 chars = ~6 seconds to send)
curl -X POST \
  -H "X-API-Key: your-api-key" \
  "http://192.168.1.XXX/type?msg=$(python3 -c 'print("A"*250)')"

# Terminal 2: Immediately send another request
# (don't wait for first to complete)
curl -X POST \
  -H "X-API-Key: your-api-key" \
  http://192.168.1.XXX/led/toggle
```

**Expected Behavior:**
- First request returns 202 Accepted immediately
- Second request succeeds without waiting
- LED toggles while BLE is still sending
- Serial monitor shows chunked sending progress

**✅ PASS CRITERIA:** System remains responsive during BLE send

---

#### Test 4.2: Concurrent BLE Requests
```bash
# Terminal 1: Send message
curl -X POST \
  -H "X-API-Key: your-api-key" \
  "http://192.168.1.XXX/type?msg=First%20message"

# Terminal 2: Immediately send second message
curl -X POST \
  -H "X-API-Key: your-api-key" \
  "http://192.168.1.XXX/type?msg=Second%20message"
```

**Expected Response (second request):**
```json
{
  "error": "System busy - another operation in progress",
  "code": 10
}
```
**HTTP Status:** 409 Conflict

**Serial Monitor Shows:**
```
Typing: First message
BLE send complete
```

**✅ PASS CRITERIA:** Second message rejected, first completes successfully

---

### Test 5: Overflow-Safe Timing

#### Test 5.1: System Uptime Test
This test requires the ESP32 to run for extended periods. For quick validation:

```bash
# Watch serial monitor for clean WiFi status checks
# Should see regular "WiFi connected" checks without errors
```

**Long-term Test:**
- Leave device running for 24+ hours
- Monitor for:
  - WiFi reconnection works
  - LED flashing works
  - No crashes or hangs

**✅ PASS CRITERIA:** System stable over extended runtime

---

### Test 6: Watchdog Timer

#### Test 6.1: Watchdog is Active
Check serial monitor during startup:
```
✓ Watchdog timer enabled (30 seconds)
```

**✅ PASS CRITERIA:** Watchdog timer initialization message appears

---

#### Test 6.2: System Recovery (Advanced)
To test watchdog recovery, you would need to intentionally introduce a hang condition. This is not recommended for normal testing.

**✅ PASS CRITERIA:** Watchdog configured (verified in serial output)

---

### Test 7: Error Handling

#### Test 7.1: BLE Not Connected Error
Before pairing BLE keyboard to computer:
```bash
curl -X POST \
  -H "X-API-Key: your-api-key" \
  "http://192.168.1.XXX/type?msg=test"
```

**Expected Response:**
```json
{
  "error": "BLE keyboard not connected",
  "code": 1
}
```
**HTTP Status:** 400 Bad Request

**✅ PASS CRITERIA:** Appropriate error when BLE not connected

---

#### Test 7.2: Missing Parameter
```bash
curl -X POST \
  -H "X-API-Key: your-api-key" \
  http://192.168.1.XXX/type
```

**Expected Response:**
```json
{
  "error": "Invalid parameter",
  "code": 6
}
```
**HTTP Status:** 400 Bad Request

**✅ PASS CRITERIA:** Missing parameter detected and reported

---

### Test 8: Configuration Management

#### Test 8.1: Verify Configuration Values
Check serial monitor output during startup:
```
====================================
System Ready
====================================
BLE Device: TopoConKeyboard
HTTP Port: 80
Rate Limit: 5 req/1000ms

⚠ SECURITY: API key required in X-API-Key header
====================================
```

**✅ PASS CRITERIA:** Configuration values displayed correctly

---

### Test 9: HTTP Method Enforcement

#### Test 9.1: GET Request to POST Endpoint (Should Fail)
```bash
curl -X GET \
  -H "X-API-Key: your-api-key" \
  http://192.168.1.XXX/ctrlaltdel
```

**Expected Behavior:** Request not handled (404 or no response)

**✅ PASS CRITERIA:** POST-only endpoints reject GET requests

---

#### Test 9.2: GET Request to Root (Should Succeed)
```bash
curl -X GET http://192.168.1.XXX/
```

**Expected Response:**
```
ESP32 BLE Keyboard Remote - Secure Edition

Available endpoints:
  POST /ctrlaltdel      - Send Ctrl+Alt+Del
  POST /sleep           - Send Win+X, U, S (Sleep)
  POST /led/toggle      - Toggle LED
  POST /type?msg=TEXT   - Type text via BLE keyboard
  GET  /                - Show this help

Authentication:
  All endpoints require X-API-Key header
...
```

**✅ PASS CRITERIA:** Help text displayed without authentication

---

## Performance Tests

### Performance Test 1: Response Time
```bash
time curl -X POST \
  -H "X-API-Key: your-api-key" \
  http://192.168.1.XXX/led/toggle
```

**Expected:** < 100ms response time

**✅ PASS CRITERIA:** Quick response times

---

### Performance Test 2: Memory Stability
Monitor serial output during extended operation:

```bash
# Send 100 requests
for i in {1..100}; do
  curl -X POST \
    -H "X-API-Key: your-api-key" \
    "http://192.168.1.XXX/type?msg=Test%20$i"
  sleep 2
done
```

**Monitor:** ESP32 should remain responsive, no crashes

**✅ PASS CRITERIA:** No memory leaks or crashes after many requests

---

## Security Tests

### Security Test 1: Brute Force Protection
```bash
# Attempt rapid authentication attempts
for i in {1..100}; do
  curl -X POST \
    -H "X-API-Key: wrong-key-$i" \
    http://192.168.1.XXX/ctrlaltdel &
done
```

**Expected:** Rate limiting kicks in, system remains responsive

**✅ PASS CRITERIA:** System handles attack gracefully

---

### Security Test 2: Log Sanitization
```bash
curl -X POST \
  -H "X-API-Key: your-api-key" \
  "http://192.168.1.XXX/type?msg=Password:%20secret123"
```

**Serial Monitor Shows:**
```
Typing: Password: secret123
```

**Note:** Logs show actual content. In production, consider additional log sanitization for sensitive data.

**✅ PASS CRITERIA:** Logs readable but don't crash on special characters

---

## Integration Tests

### Integration Test 1: Full Workflow
1. Connect to WiFi ✓
2. Start BLE keyboard ✓
3. Pair with computer
4. Send authenticated request
5. Receive BLE typing on computer
6. Toggle LED
7. WiFi disconnect/reconnect
8. Verify system recovery

**✅ PASS CRITERIA:** Complete workflow succeeds

---

## Test Results Template

Use this template to record your test results:

```markdown
## Test Results - Phase 1

**Date:** 2025-11-05
**Tester:** [Your Name]
**Hardware:** ESP32 Dev Board
**Environment:** Home Network

### Test Summary
- Total Tests: 20
- Passed: __
- Failed: __
- Skipped: __

### Failed Tests
- [ ] Test X.Y: [Description]
  - Issue: [What went wrong]
  - Expected: [What should happen]
  - Actual: [What happened]

### Notes
[Any additional observations]
```

---

## Troubleshooting

### Issue: ESP32 Not Connecting to WiFi
**Solution:**
- Verify credentials in `src/secrets.h`
- Check 2.4GHz network (ESP32 doesn't support 5GHz)
- Check serial monitor for errors

### Issue: API Key Not Working
**Solution:**
- Ensure no extra spaces in secrets.h
- Header name must be exact: `X-API-Key`
- Try regenerating key

### Issue: BLE Not Pairing
**Solution:**
- Check BLE is enabled on computer
- Try forgetting and re-pairing
- Check serial monitor for BLE status

### Issue: Build Errors
**Solution:**
```bash
# Clean and rebuild
pio run --target clean
pio lib install

# Check library dependencies
pio lib list
```

---

## Success Criteria Summary

Phase 1 is successfully implemented if:
- ✅ All authentication tests pass
- ✅ All validation tests pass
- ✅ Rate limiting works correctly
- ✅ System remains responsive during BLE sends
- ✅ No crashes after 24 hours runtime
- ✅ All error codes return correctly
- ✅ Watchdog timer is active

---

## Next Steps After Testing

If all tests pass:
1. ✅ Mark Phase 1 as complete
2. ✅ Update CLAUDE.md with new features
3. ✅ Update README.md with security notes
4. ✅ Proceed to Phase 2 (Architecture Improvements)

If tests fail:
1. ❌ Document failures in test results
2. ❌ Review error messages
3. ❌ Fix issues
4. ❌ Re-test

---

## Quick Test Script

Save this as `test.sh` for quick testing:

```bash
#!/bin/bash

# Configuration
ESP32_IP="192.168.1.XXX"  # UPDATE THIS
API_KEY="your-api-key"     # UPDATE THIS

echo "=== ESP32 BLE Keyboard Test Suite ==="
echo

# Test 1: Authentication
echo "Test 1: No API Key (should fail with 401)"
curl -s -X POST http://$ESP32_IP/led/toggle | jq
echo

# Test 2: Valid Request
echo "Test 2: Valid request (should succeed)"
curl -s -X POST -H "X-API-Key: $API_KEY" http://$ESP32_IP/led/toggle | jq
echo

# Test 3: Input Validation
echo "Test 3: Empty message (should fail with 400)"
curl -s -X POST -H "X-API-Key: $API_KEY" "http://$ESP32_IP/type?msg=" | jq
echo

# Test 4: Valid Message
echo "Test 4: Valid message (should succeed)"
curl -s -X POST -H "X-API-Key: $API_KEY" "http://$ESP32_IP/type?msg=Hello" | jq
echo

echo "=== Test Suite Complete ==="
```

Run with:
```bash
chmod +x test.sh
./test.sh
```

---

## Conclusion

Comprehensive testing ensures all Phase 1 improvements work correctly and the system is significantly more secure and reliable than the original implementation.

**Estimated Testing Time:** 2-3 hours for complete suite
