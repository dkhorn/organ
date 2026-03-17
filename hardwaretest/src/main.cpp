#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "pins.h"
#include "output.h"
#include "logger.h"
#include "httpserver.h"
#include "midinote.h"
#include "midiseq.h"
#include "logger.h"
#include "timekeeping.h"
#include "midireceiver.h"
#include "midiudp.h"
#include "httpserver.h"
#include "can_bus.h"

// ---- WiFi creds ----
static const char* WIFI_SSID = "HAWI";
static const char* WIFI_PASS = "murphalurf18";

// Status LED (GPIO 38 is the RGB LED on ESP32-S3-DevKitC-1)
#define LED_PIN 38

// Telnet server for remote logging
WiFiServer telnetServer(23);
WiFiClient telnetClient;

// // Custom print that outputs to both Serial and Telnet
// class DualOutput : public Print {
// public:
//   size_t write(uint8_t c) override {
//     size_t n = Serial.write(c);
//     if (telnetClient && telnetClient.connected()) {
//       telnetClient.write(c);
//     }
//     return n;
//   }
//   size_t write(const uint8_t *buffer, size_t size) override {
//     size_t n = Serial.write(buffer, size);
//     if (telnetClient && telnetClient.connected()) {
//       telnetClient.write(buffer, size);
//     }
//     return n;
//   }
// };

// DualOutput Log;

static void wifi_connect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
    if ((millis() - t0) > 15000) {
      Serial.println(" timeout!");
      return;
    }
  }
  Serial.println(" connected!");
}

static void ota_begin() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([] {
    Log.println("OTA Start");
  });
  ArduinoOTA.onEnd([] {
    Log.println("\nOTA End");
  });
  ArduinoOTA.onProgress([](unsigned int prog, unsigned int total) {
    // Quick blink as progress indicator
    digitalWrite(LED_PIN, (prog / 16) & 1);
    Log.printf("Progress: %u%%\r", (prog * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Log.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Log.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Log.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Log.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Log.println("Receive Failed");
    else if (error == OTA_END_ERROR) Log.println("End Failed");
  });
  ArduinoOTA.begin();

  if (!MDNS.begin(OTA_HOSTNAME)) {
    Log.println("mDNS failed (OTA still works by IP)");
  } else {
    MDNS.addService("arduino", "tcp", 3232);
    Log.println("mDNS started");
  }
  
  // Start telnet server
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  Log.println("Telnet server started on port 23");
}



// ---------- Test config ----------
const long STEP_INTERVAL   = 3000;   // ms between pattern steps
const long DEBOUNCE_MS     = 40;

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

  flushOutput();
  // Serial.println("Tick");
}

// ---------- Setup / loop ----------
void setup() {
  Serial.begin(115200);

  wifi_connect();
  
  if (WiFi.status() == WL_CONNECTED) {
    ota_begin();
    httpserver_begin();
    Log.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Log.println("IP: Not connected");
  }


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

  output_begin();
  midinote_begin();
  midiseq_begin();
  midiReceiver.begin();
  midiUDP.begin();  // Start MIDI/UDP receiver on port 21928
  timekeeping.begin();
  can_bus_begin();  // Start CAN bus (TWAI) for note event broadcast

  clearAll();
  flushOutput();
  // shiftOutBytes(outBuf, NUM_BYTES);
    // Start telnet server
  telnetServer.begin();
  telnetServer.setNoDelay(true);
}

void loop() {
  handleButton();
  // updatePattern();
  if (WiFi.status() == WL_CONNECTED) {
    // Check for new telnet clients
    if (telnetServer.hasClient()) {
      // Disconnect old client if exists
      if (telnetClient) {
        telnetClient.stop();
      }
      telnetClient = telnetServer.available();
      Log.println("\nTelnet client connected");
    }
    
    // Handle OTA
    ArduinoOTA.handle();
    
    // Handle HTTP requests
    httpserver_loop();
  }
}
