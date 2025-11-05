#include "Arduino.h"

// Global mock state
unsigned long mock_millis_value = 0;
uint8_t mock_pin_states[50] = {0};
MockSerial Serial;
