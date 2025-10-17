#include <Arduino.h>
#include <BleKeyboard.h>
#include <WiFi.h>
#include <WebServer.h>

// -------------------- CONFIG --------------------
const char *WIFI_SSID = "KirbyNet";
const char *WIFI_PASS = "4806508554";
const char *API_KEY = ""; // optional – set "" to disable
const int HTTP_PORT = 80;
// ------------------------------------------------

BleKeyboard bleKeyboard("CtrlAltDel", "Topo Consulting LLC", 100);
WebServer server(HTTP_PORT);

const int BUTTON_PIN = 0;       // BOOT button (GPIO0)
const int LONG_PRESS_MS = 2000; // 2 seconds
static bool lastState = HIGH;
static unsigned long pressStart = 0;
static bool longPressTriggered = false;

// === Key press helpers ===
void sendCtrlAltDel()
{
  Serial.println("Sending Ctrl + Alt + Del...");
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press(KEY_LEFT_ALT);
  bleKeyboard.press(KEY_DELETE);
  delay(100);
  bleKeyboard.releaseAll();
  delay(3000);
  if (bleKeyboard.isConnected())
  {
    bleKeyboard.print("Blessed are the peacemakers!");
  }
}

void sendSleepCombo()
{
  Serial.println("Sending Win+X -> U -> S (Sleep)...");
  bleKeyboard.press(KEY_LEFT_GUI);
  bleKeyboard.press('x');
  bleKeyboard.releaseAll();
  delay(500);

  bleKeyboard.press('u');
  bleKeyboard.releaseAll();
  delay(500);

  bleKeyboard.press('s');
  bleKeyboard.releaseAll();
}

// === HTTP helpers ===
bool checkApiKey()
{
  if (strlen(API_KEY) == 0)
    return true;
  if (server.hasHeader("X-API-Key"))
    return server.header("X-API-Key") == API_KEY;
  return false;
}

void handleRoot()
{
  server.send(200, "text/plain",
              "ESP32 BLE Keyboard API is running.\n"
              "Endpoints:\n"
              "GET /ctrlaltdel\n"
              "GET /sleep\n");
}

void handleCtrlAltDel()
{
  if (!checkApiKey())
  {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }

  if (bleKeyboard.isConnected())
  {
    sendCtrlAltDel();
    server.send(200, "application/json", "{\"status\":\"CtrlAltDel sent\"}");
  }
  else
  {
    server.send(503, "application/json", "{\"error\":\"BLE not connected\"}");
  }
}

void handleSleep()
{
  if (!checkApiKey())
  {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }

  if (bleKeyboard.isConnected())
  {
    sendSleepCombo();
    server.send(200, "application/json", "{\"status\":\"Sleep combo sent\"}");
  }
  else
  {
    server.send(503, "application/json", "{\"error\":\"BLE not connected\"}");
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Starting BLE Keyboard...");
  bleKeyboard.begin();

  Serial.printf("Connecting to Wi-Fi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Register routes
  server.on("/", handleRoot);
  server.on("/ctrlaltdel", handleCtrlAltDel);
  server.on("/sleep", handleSleep);

  // Handle undefined routes
  server.onNotFound([]()
                    { server.send(404, "text/plain", "Not found"); });

  server.begin();
  Serial.printf("HTTP server started on port %d\n", HTTP_PORT);
}

void loop()
{
  // Handle HTTP
  server.handleClient();

  // Handle physical button (same logic as before)
  bool currentState = digitalRead(BUTTON_PIN);
  if (lastState == HIGH && currentState == LOW)
  {
    pressStart = millis();
    longPressTriggered = false;
  }

  if (currentState == LOW && !longPressTriggered)
  {
    if (millis() - pressStart >= LONG_PRESS_MS)
    {
      if (bleKeyboard.isConnected())
        sendSleepCombo();
      longPressTriggered = true;
    }
  }

  if (lastState == LOW && currentState == HIGH)
  {
    unsigned long pressDuration = millis() - pressStart;
    if (!longPressTriggered && pressDuration < LONG_PRESS_MS)
    {
      if (bleKeyboard.isConnected())
        sendCtrlAltDel();
    }
  }

  lastState = currentState;
  delay(50);
}
