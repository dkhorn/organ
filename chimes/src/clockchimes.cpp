#include "clockchimes.h"
#include "timekeeping.h"
#include "midiseq.h"
#include "noterepeater.h"
#include "logger.h"
#include <Preferences.h>

const char* ClockChimes::NVS_NAMESPACE = "clockchimes";

// Global instance
ClockChimes clockChimes;

// Preferences object for NVS access
static Preferences chimePrefs;

void ClockChimes::begin() {
    Log.println("Initializing clock chimes...");
    
    // Initialize state
    enabled = false;
    tuneNumber = 5;  // Test Tune by default
    hourStrikeEnabled = true;
    hourNotes[0] = 70;  // C4
    hourNotes[1] = 77;  // E4
    hourNotes[2] = 0;  // G4
    tuneTempo = 120;  // BPM
    tuneVelocity = 127;  // Full velocity by default
    hourStrikeInterval = 2000;  // 2 seconds between strikes
    hourVelocity = 127;  // Full velocity by default
    quietModeScale = 64;  // 50% volume during quiet hours
    quietModeStartHour = 20;  // 8 PM
    quietModeEndHour = 7;  // 7 AM
    silenceStartHour = 22;  // Disabled by default
    silenceEndHour = 7;  // Disabled by default
    
    lastMinute = 255;  // Force initial check
    lastHour = 255;
    chimeInProgress = false;
    pendingHourStrike = false;
    pendingStrikeCount = 0;
    
    // Load saved settings
    loadSettings();
    
    Log.printf("Clock chimes: %s\n", enabled ? "ENABLED" : "DISABLED");
    Log.printf("Tune: %s\n", getTuneName(tuneNumber));
    Log.printf("Tune tempo: %d BPM\n", tuneTempo);
    Log.printf("Tune velocity: %d\n", tuneVelocity);
    Log.printf("Hour strike: %s\n", hourStrikeEnabled ? "ENABLED" : "DISABLED");
    Log.printf("Hour notes: %d, %d, %d\n", hourNotes[0], hourNotes[1], hourNotes[2]);
    Log.printf("Hour strike interval: %d ms\n", hourStrikeInterval);
    Log.printf("Hour strike velocity: %d\n", hourVelocity);
    Log.printf("Quiet mode: scale=%d, hours=%d-%d\n", quietModeScale, quietModeStartHour, quietModeEndHour);
    Log.printf("Silence mode: hours=%d-%d\n", silenceStartHour, silenceEndHour);
}

void ClockChimes::update() {
    // Always handle chime completion and pending strikes, regardless of enabled state
    if (chimeInProgress) {
        // Check if sequence finished
        if (!midiseq_is_playing()) {
            chimeInProgress = false;
            Log.println("Chime sequence finished");
            
            // If hour strike is pending, trigger it now
            if (pendingHourStrike) {
                pendingHourStrike = false;
                strikeHour(pendingStrikeCount);
            }
        }
        return;
    }
    
    // Only check for new time-based chimes if enabled
    if (!enabled) return;
    
    // Get current time
    struct tm timeinfo;
    if (!timekeeping.getLocalTime(&timeinfo)) {
        return;  // Time not synchronized yet
    }
    
    uint8_t currentMinute = timeinfo.tm_min;
    uint8_t currentHour = timeinfo.tm_hour;
    
    // Check if we're in silence mode
    if (isInSilenceMode(currentHour)) {
        lastMinute = currentMinute;
        lastHour = currentHour;
        return;  // Silence mode - no chimes at all
    }
    
    // Check if we crossed a quarter hour boundary
    if (currentMinute != lastMinute) {
        lastMinute = currentMinute;
        
        // Trigger chimes on quarter hours
        if (currentMinute == 15) {
            Log.println("Quarter hour chime");
            playChime(1);
        } else if (currentMinute == 30) {
            Log.println("Half hour chime");
            playChime(2);
        } else if (currentMinute == 45) {
            Log.println("Three-quarter hour chime");
            playChime(3);
        } else if (currentMinute == 0) {
            Log.println("Hour chime");
            playChime(4);
            
            // Schedule hour strike to occur after chime finishes
            if (hourStrikeEnabled && currentHour != lastHour) {
                lastHour = currentHour;
                // Convert 24-hour to 12-hour for striking
                uint8_t strikeHourNum = currentHour % 12;
                if (strikeHourNum == 0) strikeHourNum = 12;
                pendingHourStrike = true;
                pendingStrikeCount = strikeHourNum;
            }
        }
    }
}

void ClockChimes::playChime(uint8_t quarter) {
    if (tuneNumber == 0 || quarter == 0) return;
    
    uint16_t length = 0;
    const MidiEvent* sequence = ClockTunes::getSequence(tuneNumber, quarter, &length);
    
    if (sequence && length > 0) {
        // Get current hour for quiet mode check
        struct tm timeinfo;
        uint8_t currentHour = 0;
        if (timekeeping.getLocalTime(&timeinfo)) {
            currentHour = timeinfo.tm_hour;
        }
        
        // Apply quiet mode to velocity if needed
        uint8_t effectiveVelocity = applyQuietMode(tuneVelocity, currentHour);
        
        midiseq_load(sequence, length, 480, tuneTempo, 0, effectiveVelocity);  // 480 tpq, configurable BPM, no transpose, configurable velocity
        midiseq_play();
        chimeInProgress = true;
    }
}

void ClockChimes::strikeHour(uint8_t hour) {
    Log.printf("Starting hour strike: %d times\n", hour);
    
    // Get current hour for quiet mode check
    struct tm timeinfo;
    uint8_t currentHour = 0;
    if (timekeeping.getLocalTime(&timeinfo)) {
        currentHour = timeinfo.tm_hour;
    }
    
    // Apply quiet mode to velocity if needed
    uint8_t effectiveVelocity = applyQuietMode(hourVelocity, currentHour);
    
    // Fire and forget - noterepeater will handle the strikes
    for (int i = 0; i < 3; i++) {
        if (hourNotes[i] >= 69 && hourNotes[i] <= 91) {
            start_repeated_note(hourNotes[i], effectiveVelocity, hourStrikeInterval, hour);
        }
    }
}

void ClockChimes::manualChime(uint8_t quarter) {
    if (quarter >= 1 && quarter <= 4) {
        Log.printf("Manual chime trigger: quarter=%d\n", quarter);
        playChime(quarter);
        
        if (quarter == 4 && hourStrikeEnabled) {
            pendingHourStrike = true;
            pendingStrikeCount = 3;  // Test with 3 strikes
        }
    }
}

void ClockChimes::setEnabled(bool en) {
    enabled = en;
    Log.printf("Clock chimes %s\n", enabled ? "ENABLED" : "DISABLED");
    saveSettings();
}

bool ClockChimes::isEnabled() {
    return enabled;
}

void ClockChimes::setTune(uint8_t tune) {
    if (tune <= getNumTunes()) {
        tuneNumber = tune;
        Log.printf("Tune set to: %s\n", ClockTunes::getTuneName(tune));
        saveSettings();
    }
}

uint8_t ClockChimes::getTune() {
    return tuneNumber;
}

void ClockChimes::setHourStrikeEnabled(bool en) {
    hourStrikeEnabled = en;
    Log.printf("Hour strike %s\n", hourStrikeEnabled ? "ENABLED" : "DISABLED");
    saveSettings();
}

bool ClockChimes::isHourStrikeEnabled() {
    return hourStrikeEnabled;
}

void ClockChimes::setHourNote(uint8_t index, uint8_t midiNote) {
    if (index < 3) {
        hourNotes[index] = midiNote;
        Log.printf("Hour note %d set to MIDI %d\n", index, midiNote);
        saveSettings();
    }
}

uint8_t ClockChimes::getHourNote(uint8_t index) {
    if (index < 3) {
        return hourNotes[index];
    }
    return 255;
}

void ClockChimes::setTuneTempo(uint16_t bpm) {
    tuneTempo = bpm;
    Log.printf("Tune tempo set to %d BPM\n", tuneTempo);
    saveSettings();
}

uint16_t ClockChimes::getTuneTempo() {
    return tuneTempo;
}

void ClockChimes::setTuneVelocity(uint8_t velocity) {
    if (velocity >= 1 && velocity <= 127) {
        tuneVelocity = velocity;
        Log.printf("Tune velocity set to %d\n", tuneVelocity);
        saveSettings();
    }
}

uint8_t ClockChimes::getTuneVelocity() {
    return tuneVelocity;
}

void ClockChimes::setHourStrikeInterval(uint32_t intervalMs) {
    hourStrikeInterval = intervalMs;
    Log.printf("Hour strike interval set to %d ms\n", hourStrikeInterval);
    saveSettings();
}

uint32_t ClockChimes::getHourStrikeInterval() {
    return hourStrikeInterval;
}

void ClockChimes::setHourVelocity(uint8_t velocity) {
    if (velocity >= 1 && velocity <= 127) {
        hourVelocity = velocity;
        Log.printf("Hour strike velocity set to %d\n", hourVelocity);
        saveSettings();
    }
}

uint8_t ClockChimes::getHourVelocity() {
    return hourVelocity;
}

void ClockChimes::setQuietModeScale(uint8_t scale) {
    if (scale <= 127) {
        quietModeScale = scale;
        Log.printf("Quiet mode scale set to %d\n", quietModeScale);
        saveSettings();
    }
}

uint8_t ClockChimes::getQuietModeScale() {
    return quietModeScale;
}

void ClockChimes::setQuietModeStartHour(uint8_t hour) {
    quietModeStartHour = hour;
    Log.printf("Quiet mode start hour set to %d\n", quietModeStartHour);
    saveSettings();
}

uint8_t ClockChimes::getQuietModeStartHour() {
    return quietModeStartHour;
}

void ClockChimes::setQuietModeEndHour(uint8_t hour) {
    quietModeEndHour = hour;
    Log.printf("Quiet mode end hour set to %d\n", quietModeEndHour);
    saveSettings();
}

uint8_t ClockChimes::getQuietModeEndHour() {
    return quietModeEndHour;
}

void ClockChimes::setSilenceStartHour(uint8_t hour) {
    silenceStartHour = hour;
    Log.printf("Silence start hour set to %d\n", silenceStartHour);
    saveSettings();
}

uint8_t ClockChimes::getSilenceStartHour() {
    return silenceStartHour;
}

void ClockChimes::setSilenceEndHour(uint8_t hour) {
    silenceEndHour = hour;
    Log.printf("Silence end hour set to %d\n", silenceEndHour);
    saveSettings();
}

uint8_t ClockChimes::getSilenceEndHour() {
    return silenceEndHour;
}

bool ClockChimes::isInSilenceMode(uint8_t currentHour) {
    if (silenceStartHour >= 24 || silenceEndHour >= 24) return false;
    
    if (silenceStartHour < silenceEndHour) {
        // Normal range: e.g., 23-7 means NOT in silence from 23:00-6:59
        return currentHour >= silenceStartHour && currentHour < silenceEndHour;
    } else if (silenceStartHour > silenceEndHour) {
        // Wraparound: e.g., 23-7 means in silence from 23:00-6:59
        return currentHour >= silenceStartHour || currentHour < silenceEndHour;
    }
    return false;  // Start == End means disabled
}

bool ClockChimes::isInQuietMode(uint8_t currentHour) {
    if (quietModeStartHour >= 24 || quietModeEndHour >= 24) return false;
    
    if (quietModeStartHour < quietModeEndHour) {
        // Normal range
        return currentHour >= quietModeStartHour && currentHour < quietModeEndHour;
    } else if (quietModeStartHour > quietModeEndHour) {
        // Wraparound
        return currentHour >= quietModeStartHour || currentHour < quietModeEndHour;
    }
    return false;
}

uint8_t ClockChimes::applyQuietMode(uint8_t baseVelocity, uint8_t currentHour) {
    if (!isInQuietMode(currentHour)) {
        return baseVelocity;
    }
    
    // Scale velocity: vel_out = (vel_in * quietModeScale) / 127
    uint16_t scaled = ((uint16_t)baseVelocity * quietModeScale) / 127;
    return (uint8_t)scaled;
}

const char* ClockChimes::getTuneName(uint8_t tuneNumber) {
    return ClockTunes::getTuneName(tuneNumber);
}

uint8_t ClockChimes::getNumTunes() {
    return ClockTunes::getNumTunes();
}

void ClockChimes::saveSettings() {
    chimePrefs.begin(NVS_NAMESPACE, false);
    
    chimePrefs.putBool("enabled", enabled);
    chimePrefs.putUChar("tune", tuneNumber);
    chimePrefs.putBool("hourStrike", hourStrikeEnabled);
    chimePrefs.putUChar("hourNote0", hourNotes[0]);
    chimePrefs.putUChar("hourNote1", hourNotes[1]);
    chimePrefs.putUChar("hourNote2", hourNotes[2]);
    chimePrefs.putUShort("tuneTempo", tuneTempo);
    chimePrefs.putUChar("tuneVelocity", tuneVelocity);
    chimePrefs.putUInt("strikeInterval", hourStrikeInterval);
    chimePrefs.putUChar("hourVelocity", hourVelocity);
    chimePrefs.putUChar("quietScale", quietModeScale);
    chimePrefs.putUChar("quietStart", quietModeStartHour);
    chimePrefs.putUChar("quietEnd", quietModeEndHour);
    chimePrefs.putUChar("silenceStart", silenceStartHour);
    chimePrefs.putUChar("silenceEnd", silenceEndHour);
    
    chimePrefs.end();
    Log.println("Clock chime settings saved to NVS");
}

void ClockChimes::loadSettings() {
    chimePrefs.begin(NVS_NAMESPACE, true);  // Read-only
    
    enabled = chimePrefs.getBool("enabled", true);
    tuneNumber = chimePrefs.getUChar("tune", 5);  // Test Tune by default
    hourStrikeEnabled = chimePrefs.getBool("hourStrike", true);
    hourNotes[0] = chimePrefs.getUChar("hourNote0", 70);  // C4
    hourNotes[1] = chimePrefs.getUChar("hourNote1", 77);  // E4
    hourNotes[2] = chimePrefs.getUChar("hourNote2", 0);  // Off
    tuneTempo = chimePrefs.getUShort("tuneTempo", 120);  // 120 BPM
    tuneVelocity = chimePrefs.getUChar("tuneVelocity", 127);  // Full velocity
    hourStrikeInterval = chimePrefs.getUInt("strikeInterval", 2000);  // 2 seconds
    hourVelocity = chimePrefs.getUChar("hourVelocity", 127);  // Full velocity
    quietModeScale = chimePrefs.getUChar("quietScale", 64);  // 50% volume
    quietModeStartHour = chimePrefs.getUChar("quietStart", 20);  // 8 PM
    quietModeEndHour = chimePrefs.getUChar("quietEnd", 7);  // 7 AM
    silenceStartHour = chimePrefs.getUChar("silenceStart", 22);  // 10 PM
    silenceEndHour = chimePrefs.getUChar("silenceEnd", 7);  // 7 AM
    
    chimePrefs.end();
    Log.println("Clock chime settings loaded from NVS");
}

void ClockChimes::resetSettings() {
    chimePrefs.begin(NVS_NAMESPACE, false);
    chimePrefs.clear();
    chimePrefs.end();
    
    // Reset to defaults
    enabled = true;
    tuneNumber = 5;
    hourStrikeEnabled = true;
    hourNotes[0] = 70;
    hourNotes[1] = 77;
    hourNotes[2] = 0;
    tuneTempo = 120;
    tuneVelocity = 127;
    hourStrikeInterval = 2000;
    hourVelocity = 127;
    quietModeScale = 64;
    quietModeStartHour = 20;
    quietModeEndHour = 7;
    silenceStartHour = 22;
    silenceEndHour = 7;
    
    Log.println("Clock chime settings reset to defaults");
    saveSettings();
}
