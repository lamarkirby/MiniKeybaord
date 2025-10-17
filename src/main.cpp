#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <BleKeyboard.h>

// ===== BLE Keyboard =====
BleKeyboard bleKeyboard("CtrlAltDel", "Topo Consulting LLC", 100);

// ===== WiFi Config =====
const char *ssid = "KirbyNet";
const char *password = "4806508554";

// ===== Web Server =====
WebServer server(80);

// ===== LED Config =====
const int LED_PIN = 12; // Onboard LED for most ESP32 dev boards
bool ledState = false;

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

// ====== HTTP Handlers ======
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

  // Send message in 2 - character chunks 
  const int MAX_CHUNK = 2;
  for (int i = 0; i < msg.length(); i += MAX_CHUNK)
  {
    String chunk = msg.substring(i, i + MAX_CHUNK);
    bleKeyboard.print(chunk);
    delay(200); // small delay between chunks
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
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.println(WiFi.localIP());

  // Register HTTP routes
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
  server.handleClient();
}
