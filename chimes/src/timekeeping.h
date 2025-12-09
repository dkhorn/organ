#ifndef TIMEKEEPING_H
#define TIMEKEEPING_H

#include <Arduino.h>
#include <time.h>

/**
 * Timekeeping module with persistent settings stored in NVS.
 * Handles NTP synchronization, timezone management, and RTC.
 */
class Timekeeping {
public:
    /**
     * Initialize timekeeping system
     * Loads settings from NVS and syncs with NTP if WiFi is available
     */
    void begin();
    
    /**
     * Update time synchronization
     * Call periodically from main loop
     */
    void update();
    
    /**
     * Get current Unix timestamp
     */
    time_t getTimestamp();
    
    /**
     * Get current time as struct tm (local time)
     */
    bool getLocalTime(struct tm* timeinfo);
    
    /**
     * Get current time formatted as string
     * @param buffer Output buffer (minimum 26 bytes for full format)
     * @param format strftime format string (default: "%Y-%m-%d %H:%M:%S")
     */
    void getTimeString(char* buffer, size_t bufferSize, const char* format = "%Y-%m-%d %H:%M:%S");
    
    /**
     * Set timezone offset in seconds from UTC
     * Positive for east of UTC, negative for west
     * Example: -8*3600 for PST, -5*3600 for EST
     */
    void setTimezoneOffset(long offsetSeconds);
    
    /**
     * Get current timezone offset in seconds
     */
    long getTimezoneOffset();
    
    /**
     * Set NTP server
     */
    void setNTPServer(const char* server);
    
    /**
     * Get NTP server
     */
    const char* getNTPServer();
    
    /**
     * Force NTP synchronization
     * Returns true if sync successful
     */
    bool syncNTP();
    
    /**
     * Get time of last successful NTP sync
     */
    time_t getLastSyncTime();
    
    /**
     * Check if time has been synchronized at least once
     */
    bool isSynced();
    
    /**
     * Set manual time (Unix timestamp)
     * Use when NTP is not available
     */
    void setTime(time_t timestamp);
    
    /**
     * Save current settings to NVS
     */
    void saveSettings();
    
    /**
     * Load settings from NVS
     */
    void loadSettings();
    
    /**
     * Reset settings to defaults
     */
    void resetSettings();

private:
    // Settings stored in NVS
    char ntpServer[64];
    long timezoneOffset;  // seconds from UTC
    time_t lastSyncTime;
    
    // Runtime state
    bool synced;
    unsigned long lastSyncAttempt;
    
    static const unsigned long SYNC_INTERVAL = 3600000; // 1 hour in ms
    static const char* NVS_NAMESPACE;
};

// Global instance
extern Timekeeping timekeeping;

#endif // TIMEKEEPING_H
