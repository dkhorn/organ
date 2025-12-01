#ifndef ORGANCOMMON_H
#define ORGANCOMMON_H

#include <Print.h>

// Forward declaration
class DualOutput;

// Global Log object for dual Serial/Telnet output
extern DualOutput Log;

// Initialize common WiFi, OTA, and telnet logging
// Call this in your setup() function
void organcommon_setup(const char* wifi_ssid, const char* wifi_password,
                       const char* ota_hostname, const char* ota_password,
                       const char* app_version);

// Handle OTA updates and telnet connections
// Call this in your loop() function
void organcommon_loop();

#endif // ORGANCOMMON_H
