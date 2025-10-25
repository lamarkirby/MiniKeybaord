#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <BleKeyboard.h>
#include "secrets.h"

// ===== BLE Keyboard =====
BleKeyboard bleKeyboard("TopoConKeyboard", "Topo Consulting LLC", 100);

// ===== Web Server =====
WebServer server(80);

// ===== LED Config =====
const int LED_PIN = 12; // Onboard LED for most ESP32 dev boards
bool ledState = false;

// ===== WiFi Config =====
unsigned long lastWiFiCheck = 0;
unsigned long wifiFailTime = 0;
bool wifiConnected = false;
bool ledFlashing = false;

// ====== BLE Functions ======
void sendCtrlAltDel()
{
  if (bleKeyboard.isConnected())
  {
    Serial.println("Sending Ctrl+Alt+Del...");
    bleKeyboard.press(KEY_LEFT_CTRL);
    bleKeyboard.press(KEY_LEFT_ALT);
    bleKeyboard.press(KEY_DELETE);
    delay(100);
    bleKeyboard.releaseAll();
  }
  else
  {
    Serial.println("BLE not connected.");
  }
}

void sendSleepCombo()
{
  if (bleKeyboard.isConnected())
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
  else
  {
    Serial.println("BLE not connected.");
  }
}

// ====== WiFi Monitoring ======
void checkWiFiConnection()
{
  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED)
  {
    if (!wifiConnected)
    {
      wifiConnected = true;
      ledFlashing = false;
      digitalWrite(LED_PIN, LOW);
      Serial.println("\nWiFi reconnected!");
    }
    wifiFailTime = 0; // Reset fail timer
    lastWiFiCheck = now;
  }
  else
  {
    if (wifiConnected)
    {
      wifiConnected = false;
      wifiFailTime = now;
      Serial.println("\nWiFi disconnected!");
    }

    // If disconnected for more than 60 seconds, start flashing
    if (now - wifiFailTime > 60000)
    {
      ledFlashing = true;
    }
  }

  // Handle LED flashing (5 sec on, 5 sec off)
  if (ledFlashing)
  {
    unsigned long flashCycle = (now / 5000) % 2; // Alternates every 5 seconds
    digitalWrite(LED_PIN, flashCycle ? HIGH : LOW);
  }
}

// ====== HTTP Handlers ======
void handleRoot()
{
  String help = "ESP32 BLE Keyboard Remote\n\n";
  help += "Available endpoints:\n";
  help += "  GET /ctrlaltdel       - Send Ctrl+Alt+Del\n";
  help += "  GET /sleep            - Send Win+X, U, S (Sleep)\n";
  help += "  GET /led/toggle       - Toggle LED connected to pin " + String(LED_PIN) + "\n";
  help += "  GET /type?msg=TEXT    - Type text via BLE keyboard\n";
  help += "  GET /                 - Show this help\n";
  server.send(200, "text/plain", help);
}

void handleCtrlAlt()
{
  sendCtrlAltDel();
  server.send(200, "text/plain", "Sent Ctrl+Alt+Del");
}

void handleSleep()
{
  sendSleepCombo();
  server.send(200, "text/plain", "Sent Sleep Combo");
}

void handleLedToggle()
{
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  String msg = String("LED is now ") + (ledState ? "ON" : "OFF");
  Serial.println(msg);
  server.send(200, "text/plain", msg);
}

void handleType()
{
  if (!bleKeyboard.isConnected())
  {
    server.send(400, "text/plain", "BLE keyboard not connected");
    return;
  }

  if (!server.hasArg("msg"))
  {
    server.send(400, "text/plain", "Missing 'msg' parameter");
    return;
  }

  String msg = server.arg("msg");
  Serial.println("Typing: " + msg);

  // Send message in 4 - character chunks 
  const int MAX_CHUNK = 4;
  for (int i = 0; i < msg.length(); i += MAX_CHUNK)
  {
    String chunk = msg.substring(i, i + MAX_CHUNK);
    bleKeyboard.print(chunk);
    delay(100); // small delay between chunks
  }
  bleKeyboard.releaseAll();

  server.send(200, "text/plain", "Typed message: " + msg);
}

// ====== Setup ======
void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Start BLE keyboard
  Serial.println("Starting BLE Keyboard...");
  bleKeyboard.begin();

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 120) // 120 * 500ms = 60 seconds
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    wifiConnected = true;
    Serial.println("\nWiFi connected!");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\nWiFi connection timeout. Will retry in background.");
    wifiFailTime = millis();
    WiFi.reconnect(); // Enable WiFi reconnection in background
  }

  // Register HTTP routes
  server.on("/", handleRoot);
  server.on("/ctrlaltdel", handleCtrlAlt);
  server.on("/sleep", handleSleep);
  server.on("/led/toggle", handleLedToggle);
  server.on("/type", handleType);

  // Start HTTP server
  server.begin();
  Serial.println("HTTP server started.");
}

// ====== Loop ======
void loop()
{
  checkWiFiConnection();
  server.handleClient();
}