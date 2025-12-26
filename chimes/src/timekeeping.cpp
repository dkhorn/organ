#include "timekeeping.h"
#include "logger.h"
#include <Preferences.h>
#include <WiFi.h>

const char* Timekeeping::NVS_NAMESPACE = "timekeeping";

// Global instance
Timekeeping timekeeping;

// Preferences object for NVS access
static Preferences prefs;

void Timekeeping::begin() {
    Log.println("Initializing timekeeping...");
    
    // Initialize default values
    synced = false;
    lastSyncAttempt = 0;
    lastSyncTime = 0;
    lastSyncSuccessful = false;
    timezoneOffset = -18000;  // Eastern Time (UTC-5)
    strcpy(ntpServer, "pool.ntp.org");
    
    // Load saved settings from NVS
    loadSettings();
    
    // Configure timezone
    String tzEnv = "UTC" + String(timezoneOffset);
    setenv("TZ", tzEnv.c_str(), 1);
    tzset();
    
    Log.printf("Timezone offset: %ld seconds\n", timezoneOffset);
    Log.printf("NTP server: %s\n", ntpServer);
    
    // Configure NTP
    configTime(0, 0, ntpServer);  // GMT offset 0, daylight offset 0 (we handle it ourselves)
    
    // Attempt initial sync if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        syncNTP();
    }
}

void Timekeeping::update() {
    // Periodic NTP sync if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        unsigned long now = millis();
        // Use adaptive sync interval: daily if last sync succeeded, every minute if failed
        unsigned long syncInterval = lastSyncSuccessful ? SYNC_INTERVAL_SUCCESS : SYNC_INTERVAL_FAILURE;
        if (now - lastSyncAttempt >= syncInterval) {
            syncNTP();
        }
    }
}

time_t Timekeeping::getTimestamp() {
    return time(nullptr);
}

bool Timekeeping::getLocalTime(struct tm* timeinfo) {
    time_t now = getTimestamp();
    if (now < 100000) {  // Not synchronized yet
        return false;
    }
    
    // Apply timezone offset
    now += timezoneOffset;
    gmtime_r(&now, timeinfo);
    return true;
}

void Timekeeping::getTimeString(char* buffer, size_t bufferSize, const char* format) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        strftime(buffer, bufferSize, format, &timeinfo);
    } else {
        snprintf(buffer, bufferSize, "Not synchronized");
    }
}

void Timekeeping::setTimezoneOffset(long offsetSeconds) {
    timezoneOffset = offsetSeconds;
    
    // Update system timezone
    String tzEnv = "UTC" + String(offsetSeconds);
    setenv("TZ", tzEnv.c_str(), 1);
    tzset();
    
    Log.printf("Timezone offset set to %ld seconds\n", offsetSeconds);
    saveSettings();
}

long Timekeeping::getTimezoneOffset() {
    return timezoneOffset;
}

void Timekeeping::setNTPServer(const char* server) {
    strncpy(ntpServer, server, sizeof(ntpServer) - 1);
    ntpServer[sizeof(ntpServer) - 1] = '\0';
    
    // Reconfigure NTP
    configTime(0, 0, ntpServer);
    
    Log.printf("NTP server set to %s\n", ntpServer);
    saveSettings();
}

const char* Timekeeping::getNTPServer() {
    return ntpServer;
}

bool Timekeeping::syncNTP() {
    Log.println("Attempting NTP sync...");
    lastSyncAttempt = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
        lastSyncSuccessful = false;
        Log.println("NTP sync failed: WiFi not connected (will retry in 1 minute)");
        return false;
    }
    
    // Wait for time sync (timeout after 5 seconds)
    int retries = 0;
    while (time(nullptr) < 100000 && retries < 50) {
        delay(100);
        retries++;
    }
    
    time_t now = time(nullptr);
    if (now > 100000) {  // Successfully synchronized
        synced = true;
        lastSyncTime = now;
        lastSyncSuccessful = true;
        
        char timeStr[32];
        getTimeString(timeStr, sizeof(timeStr));
        Log.printf("NTP sync successful: %s (next sync in 24h)\n", timeStr);
        
        // Save last sync time to NVS
        prefs.begin(NVS_NAMESPACE, false);
        prefs.putULong("lastSync", lastSyncTime);
        prefs.end();
        
        return true;
    }
    
    lastSyncSuccessful = false;
    Log.println("NTP sync failed: timeout (will retry in 1 minute)");
    return false;
}

time_t Timekeeping::getLastSyncTime() {
    return lastSyncTime;
}

void Timekeeping::onWiFiReconnect() {
    Log.println("WiFi reconnected - triggering time sync...");
    // Reset sync attempt timer to trigger immediate sync on next update()
    lastSyncAttempt = 0;
}

bool Timekeeping::isSynced() {
    return synced && (time(nullptr) > 100000);
}

void Timekeeping::setTime(time_t timestamp) {
    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
    
    synced = true;
    lastSyncTime = timestamp;
    
    char timeStr[32];
    getTimeString(timeStr, sizeof(timeStr));
    Log.printf("Time manually set: %s\n", timeStr);
}

void Timekeeping::saveSettings() {
    prefs.begin(NVS_NAMESPACE, false);
    
    prefs.putString("ntpServer", ntpServer);
    prefs.putLong("tzOffset", timezoneOffset);
    prefs.putULong("lastSync", lastSyncTime);
    
    prefs.end();
    Log.println("Timekeeping settings saved to NVS");
}

void Timekeeping::loadSettings() {
    prefs.begin(NVS_NAMESPACE, true);  // Read-only
    
    // Load NTP server (default: pool.ntp.org)
    String savedServer = prefs.getString("ntpServer", "pool.ntp.org");
    strncpy(ntpServer, savedServer.c_str(), sizeof(ntpServer) - 1);
    ntpServer[sizeof(ntpServer) - 1] = '\0';
    
    // Load timezone offset (default: -18000 = Eastern Time UTC-5)
    timezoneOffset = prefs.getLong("tzOffset", -18000);
    
    // Load last sync time
    lastSyncTime = prefs.getULong("lastSync", 0);
    if (lastSyncTime > 100000) {
        synced = true;
        Log.printf("Previous sync time loaded: %lu\n", lastSyncTime);
    }
    
    prefs.end();
    Log.println("Timekeeping settings loaded from NVS");
}

void Timekeeping::resetSettings() {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
    
    // Reset to defaults
    timezoneOffset = -18000;  // Eastern Time
    strcpy(ntpServer, "pool.ntp.org");
    lastSyncTime = 0;
    synced = false;
    
    Log.println("Timekeeping settings reset to defaults");
    saveSettings();
}
