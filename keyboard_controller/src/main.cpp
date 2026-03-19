#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "pins.h"
#include "key_scanner.h"
#include "can_bus.h"
#include "logger.h"
#include "httpserver.h"

// ---- WiFi credentials ----
static const char* WIFI_SSID = "HAWI";
static const char* WIFI_PASS = "murphalurf18";

// Status LED (GPIO 38 is the RGB LED on ESP32-S3-DevKitC-1)
#define LED_PIN 38

// Telnet server for remote logging
WiFiServer telnetServer(23);
WiFiClient telnetClient;

// ---- WiFi ----
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

// ---- OTA / mDNS ----
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
        digitalWrite(LED_PIN, (prog / 16) & 1);
        Log.printf("Progress: %u%%\r", (prog * 100) / total);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Log.printf("OTA Error[%u]: ", error);
        if      (error == OTA_AUTH_ERROR)    Log.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)   Log.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Log.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Log.println("Receive Failed");
        else if (error == OTA_END_ERROR)     Log.println("End Failed");
    });
    ArduinoOTA.begin();

    if (!MDNS.begin(OTA_HOSTNAME)) {
        Log.println("mDNS failed (OTA still works by IP)");
    } else {
        MDNS.addService("arduino", "tcp", 3232);
        Log.println("mDNS started");
    }

    telnetServer.begin();
    telnetServer.setNoDelay(true);
    Log.println("Telnet server started on port 23");
}

// ---- Setup ----
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

    can_bus_begin();
    key_scanner_begin();

    Log.printf("Keyboard controller ready  (hw_id=%d  CAN ch=%d)\n",
               key_scanner_get_hardware_id(), key_scanner_get_can_channel());
}

// ---- Loop ----
void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        // Accept new telnet client
        if (telnetServer.hasClient()) {
            if (telnetClient) telnetClient.stop();
            telnetClient = telnetServer.available();
            Log.println("\nTelnet client connected");
        }
        ArduinoOTA.handle();
        httpserver_loop();
    }

    // Scan keys and send CAN note messages on any change
    key_scanner_update();
}
