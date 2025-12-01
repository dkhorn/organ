#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <OrganCommon.h>
#include <OrganPins.h>
#include "constants.h"


// ---------- Test config ----------
const int  NUM_CHANNELS    = 48;
const int  NUM_BYTES       = (NUM_CHANNELS + 7) / 8;   // 3 bytes
const long STEP_INTERVAL   = 150;   // ms between pattern steps
const long DEBOUNCE_MS     = 40;

uint8_t outBuf[NUM_BYTES];

enum TestMode {
  MODE_ALL_OFF = 0,
  MODE_ALL_ON,
  MODE_WALKING_ONE,
  MODE_WALKING_ZERO,
  MODE_ALT_PATTERN,
  MODE_MAX
};

TestMode mode = MODE_WALKING_ONE;
int      stepIndex = 0;
uint32_t lastStepMs   = 0;
uint32_t lastButtonMs = 0;
bool     lastButtonState = HIGH;   // INPUT_PULLUP

// ---------- Helpers ----------
void shiftOutBytes(const uint8_t *buf, int len) {
  digitalWrite(PIN_LATCH, LOW);

  // buf[0] = channels 0..7, buf[1] = 8..15, buf[2] = 16..23
  // MSB-first inside each byte.
  for (int i = len - 1; i >= 0; --i) {
    uint8_t b = buf[i];
    for (int bit = 7; bit >= 0; --bit) {
      digitalWrite(PIN_SCK, LOW);
      digitalWrite(PIN_MOSI, (b & (1 << bit)) ? HIGH : LOW);
      digitalWrite(PIN_SCK, HIGH);
    }
  }

  digitalWrite(PIN_LATCH, HIGH);
}

void clearAll() {
  memset(outBuf, 0x00, NUM_BYTES);
}

void setAll(bool v) {
  memset(outBuf, v ? 0xFF : 0x00, NUM_BYTES);
}

void setChannel(int idx, bool v) {
  if (idx < 0 || idx >= NUM_CHANNELS) return;
  int byteIndex = idx / 8;
  int bitIndex  = idx % 8;   // channel 0 = bit0 of buf[0]
  if (v) outBuf[byteIndex] |=  (1 << bitIndex);
  else  outBuf[byteIndex] &= ~(1 << bitIndex);
}

// ---------- Button handling ----------
void handleButton() {
  bool raw = digitalRead(PIN_BUTTON);  // LOW when pressed
  uint32_t now = millis();

  if (raw != lastButtonState && (now - lastButtonMs) > DEBOUNCE_MS) {
    lastButtonMs = now;
    lastButtonState = raw;

    if (raw == LOW) {  // on press
      mode = (TestMode)((int(mode) + 1) % MODE_MAX);
      stepIndex = 0;
      Serial.print("Mode -> ");
      Serial.println((int)mode);
    }
  }
}

// ---------- Pattern update ----------
void updatePattern() {
  uint32_t now = millis();
  if (now - lastStepMs < STEP_INTERVAL) return;
  lastStepMs = now;

  switch (mode) {
    case MODE_ALL_OFF:
      clearAll();
      break;

    case MODE_ALL_ON:
      setAll(true);
      break;

    case MODE_WALKING_ONE:
      clearAll();
      setChannel(stepIndex, true);
      stepIndex = (stepIndex + 1) % NUM_CHANNELS;
      break;

    case MODE_WALKING_ZERO:
      setAll(true);
      setChannel(stepIndex, false);
      stepIndex = (stepIndex + 1) % NUM_CHANNELS;
      break;

    case MODE_ALT_PATTERN:
      // 0101... pattern, flips phase each step
      for (int i = 0; i < NUM_CHANNELS; ++i) {
        bool bit = ((i & 1) ^ (stepIndex & 1)) == 0;
        setChannel(i, bit);
      }
      stepIndex++;
      break;
  }

  shiftOutBytes(outBuf, NUM_BYTES);
  Serial.println("Tick");
}

// ---------- Setup / loop ----------
void setup() {
  // WiFi credentials - move these to constants.h or pass as needed
  static const char* WIFI_SSID = "HAWI";
  static const char* WIFI_PASS = "murphalurf18";
  
  organcommon_setup(WIFI_SSID, WIFI_PASS, OTA_HOSTNAME, OTA_PASSWORD, APP_VERSION);

  pinMode(PIN_MOSI,   OUTPUT);
  pinMode(PIN_SCK,    OUTPUT);
  pinMode(PIN_LATCH,  OUTPUT);
  pinMode(PIN_OE_N,   OUTPUT);
  pinMode(PIN_CLR_N,  OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // Safe defaults
  digitalWrite(PIN_MOSI,  LOW);
  digitalWrite(PIN_SCK,   LOW);
  digitalWrite(PIN_LATCH, LOW);
  digitalWrite(PIN_OE_N,  LOW);   // active low, enable outputs
  digitalWrite(PIN_CLR_N, HIGH);  // active low, so HIGH = normal

  clearAll();
  shiftOutBytes(outBuf, NUM_BYTES);
}

void loop() {
  handleButton();
  updatePattern();
  organcommon_loop();
}
