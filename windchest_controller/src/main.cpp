#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "pins.h"
#include "output.h"
#include "logger.h"
#include "httpserver.h"
#include "midinote.h"
#include "midireceiver.h"
#include "midiudp.h"
#include "config.h"

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
  
  Log.println("Telnet server started on port 23");
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


  config_begin();  // load NVS settings before output/midi modules init

  pinMode(PIN_MOSI,   OUTPUT);
  pinMode(PIN_SCK,    OUTPUT);
  pinMode(PIN_LATCH,  OUTPUT);
  pinMode(PIN_OE_N,   OUTPUT);
  pinMode(PIN_CLR_N,  OUTPUT);

  // Safe defaults
  digitalWrite(PIN_MOSI,  LOW);
  digitalWrite(PIN_SCK,   LOW);
  digitalWrite(PIN_LATCH, LOW);
  digitalWrite(PIN_OE_N,  LOW);   // active low, enable outputs
  digitalWrite(PIN_CLR_N, HIGH);  // active low, so HIGH = normal

  output_begin();
  midinote_begin();
  midiReceiver.begin();
  midiUDP.begin();  // Start MIDI/UDP receiver on port 21928

  clearAll();
  flushOutput();

  // Start telnet server
  telnetServer.begin();
  telnetServer.setNoDelay(true);
}

void loop() {
  midiReceiver.update();
  midiUDP.update();

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
