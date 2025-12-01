#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "OrganCommon.h"

// Status LED (GPIO 38 is the RGB LED on ESP32-S3-DevKitC-1)
#define LED_PIN 38

// Telnet server for remote logging
WiFiServer telnetServer(23);
WiFiClient telnetClient;

// Custom print that outputs to both Serial and Telnet
class DualOutput : public Print {
public:
public:
  size_t write(uint8_t c) override {
    size_t n = Serial.write(c);
    if (telnetClient && telnetClient.connected()) {
      telnetClient.write(c);
    }
    return n;
  }
  size_t write(const uint8_t *buffer, size_t size) override {
    size_t n = Serial.write(buffer, size);
    if (telnetClient && telnetClient.connected()) {
      telnetClient.write(buffer, size);
    }
    return n;
  }
};

DualOutput Log;

static void wifi_connect(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
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

static void ota_begin(const char* hostname, const char* password) {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(password);
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

  if (!MDNS.begin(hostname)) {
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

void organcommon_setup(const char* wifi_ssid, const char* wifi_password, 
                       const char* ota_hostname, const char* ota_password,
                       const char* app_version) {
  
  Serial.begin(115200);
  delay(200);

  Log.printf("\n\n=== %s ===\n", ota_hostname);
  
  wifi_connect(wifi_ssid, wifi_password);
  
  if (WiFi.status() == WL_CONNECTED) {
    ota_begin(ota_hostname, ota_password);
    Log.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Log.println("IP: Not connected");
  }
  
  Log.printf("Host: %s\n", ota_hostname);
  Log.printf("Version: %s\n\n", app_version);
}

void organcommon_loop() {
  // Handle telnet connections
  if (WiFi.status() == WL_CONNECTED) {
    // Check for new telnet clients
    if (telnetServer.hasClient()) {
      // Disconnect old client if exists
      if (telnetClient) {
        telnetClient.stop();
      }
      telnetClient = telnetServer.accept();
      Log.println("\nTelnet client connected");
    }
    
    // Handle OTA
    ArduinoOTA.handle();
  }
}
