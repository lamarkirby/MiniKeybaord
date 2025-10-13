#include <Arduino.h>
#include <BleKeyboard.h>

BleKeyboard bleKeyboard("CtrlAltDel", "Topo Consulting llc", 100);

const int BUTTON_PIN = 0;       // BOOT button (GPIO0)
const int LONG_PRESS_MS = 2000; // 2-second threshold for long press

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Starting BLE Keyboard...");
  bleKeyboard.begin();
}

void sendCtrlAltDel()
{
  Serial.println("Sending Ctrl + Alt + Del...");
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press(KEY_LEFT_ALT);
  bleKeyboard.press(KEY_DELETE);
  delay(100);
  bleKeyboard.releaseAll();
}

void sendSleepCombo()
{
  Serial.println("Sending Win+X -> U -> S (Sleep)...");
  // Open Power User Menu
  bleKeyboard.press(KEY_LEFT_GUI);
  bleKeyboard.press('x');
  bleKeyboard.releaseAll();
  delay(500);

  // Open shutdown submenu
  bleKeyboard.press('u');
  bleKeyboard.releaseAll();
  delay(500);

  // Select Sleep
  bleKeyboard.press('s');
  bleKeyboard.releaseAll();
}

static bool lastState = HIGH;
bool currentState = digitalRead(BUTTON_PIN);
static unsigned long pressStart = 0;
static bool longPressTriggered = false;

void loop()
{
  currentState = digitalRead(BUTTON_PIN);

  if (lastState == HIGH && currentState == LOW)
  {
    // Button just pressed
    pressStart = millis();
    longPressTriggered = false;
  }

  // While held down, check for long press
  if (currentState == LOW && !longPressTriggered)
  {
    if (millis() - pressStart >= LONG_PRESS_MS)
    {
      if (bleKeyboard.isConnected())
      {
        sendSleepCombo();
      }
      longPressTriggered = true; // Mark as handled
    }
  }

  // When released, if not long press, treat as short press
  if (lastState == LOW && currentState == HIGH)
  {
    unsigned long pressDuration = millis() - pressStart;
    if (!longPressTriggered && pressDuration < LONG_PRESS_MS)
    {
      if (bleKeyboard.isConnected())
      {
        sendCtrlAltDel();
      }
    }
  }

  lastState = currentState;
  delay(50); // debounce
}
