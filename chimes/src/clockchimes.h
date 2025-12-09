#ifndef CLOCKCHIMES_H
#define CLOCKCHIMES_H

#include <Arduino.h>
#include "midiseq.h"

/**
 * Clock chimes module - plays Westminster/Whittington-style chimes on the quarter hour
 * and strikes the hour like a grandfather clock.
 * 
 * Settings are persisted in NVS.
 */
class ClockChimes {
public:
    /**
     * Initialize clock chimes system
     * Loads settings from NVS
     */
    void begin();
    
    /**
     * Update clock chimes - call from main loop
     * Checks time and triggers chimes at appropriate moments
     */
    void update();
    
    /**
     * Enable/disable master clock chime function
     */
    void setEnabled(bool enabled);
    bool isEnabled();
    
    /**
     * Set tune number (0 = disabled, 1-4 = different tunes)
     */
    void setTune(uint8_t tuneNumber);
    uint8_t getTune();
    
    /**
     * Enable/disable hour striking
     */
    void setHourStrikeEnabled(bool enabled);
    bool isHourStrikeEnabled();
    
    /**
     * Set notes for hour striking (1-3 notes can be played simultaneously)
     * Set note to 255 to disable that voice
     */
    void setHourNote(uint8_t index, uint8_t midiNote);
    uint8_t getHourNote(uint8_t index);
    
    /**
     * Set tune tempo in BPM (beats per minute)
     */
    void setTuneTempo(uint16_t bpm);
    uint16_t getTuneTempo();
    
    /**
     * Set tune velocity (1-127, where 127 = full velocity)
     */
    void setTuneVelocity(uint8_t velocity);
    uint8_t getTuneVelocity();
    
    /**
     * Set hour strike interval in milliseconds
     */
    void setHourStrikeInterval(uint32_t intervalMs);
    uint32_t getHourStrikeInterval();
    
    /**
     * Set hour strike velocity (1-127, where 127 = full velocity)
     */
    void setHourVelocity(uint8_t velocity);
    uint8_t getHourVelocity();
    
    /**
     * Set quiet mode scale (0-127, applied to velocities during quiet hours)
     */
    void setQuietModeScale(uint8_t scale);
    uint8_t getQuietModeScale();
    
    /**
     * Set quiet mode hours (0-24, start/end hour for reduced volume)
     */
    void setQuietModeStartHour(uint8_t hour);
    uint8_t getQuietModeStartHour();
    void setQuietModeEndHour(uint8_t hour);
    uint8_t getQuietModeEndHour();
    
    /**
     * Set silence hours (0-24, start/end hour for no chimes at all)
     */
    void setSilenceStartHour(uint8_t hour);
    uint8_t getSilenceStartHour();
    void setSilenceEndHour(uint8_t hour);
    uint8_t getSilenceEndHour();
    
    /**
     * Get available tune names
     */
    static const char* getTuneName(uint8_t tuneNumber);
    static uint8_t getNumTunes();
    
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
    
    /**
     * Manually trigger a chime (for testing)
     * quarter: 0=none, 1=quarter, 2=half, 3=three-quarter, 4=hour
     */
    void manualChime(uint8_t quarter);

private:
    // Settings (stored in NVS)
    bool enabled;
    uint8_t tuneNumber;  // 0 = disabled, 1-4 = different tunes
    bool hourStrikeEnabled;
    uint8_t hourNotes[3];  // MIDI notes for hour strike (255 = disabled)
    uint16_t tuneTempo;  // BPM for tune playback
    uint8_t tuneVelocity;  // Maximum velocity (1-127)
    uint32_t hourStrikeInterval;  // Milliseconds between hour strikes
    uint8_t hourVelocity;  // Hour strike velocity (1-127)
    uint8_t quietModeScale;  // Velocity scale during quiet hours (0-127)
    uint8_t quietModeStartHour;  // Start of quiet mode (0-24)
    uint8_t quietModeEndHour;  // End of quiet mode (0-24)
    uint8_t silenceStartHour;  // Start of silence (0-24)
    uint8_t silenceEndHour;  // End of silence (0-24)
    
    // Runtime state
    uint8_t lastMinute;
    uint8_t lastHour;
    bool chimeInProgress;
    bool pendingHourStrike;
    uint8_t pendingStrikeCount;
    
    static const char* NVS_NAMESPACE;
    
    void playChime(uint8_t quarter);
    void strikeHour(uint8_t hour);
    bool isInSilenceMode(uint8_t currentHour);
    bool isInQuietMode(uint8_t currentHour);
    uint8_t applyQuietMode(uint8_t baseVelocity, uint8_t currentHour);
};

// Clock chime tune definitions
namespace ClockTunes {
    // Get tune name by number
    const char* getTuneName(uint8_t tuneNumber);
    
    // Get total number of available tunes
    uint8_t getNumTunes();
    
    // Get tune sequence for a given tune and quarter
    // tuneNumber: 1-4 (Westminster, Whittington, St. Michael's, Winchester)
    // quarter: 1=quarter, 2=half, 3=three-quarter, 4=hour
    // Returns: pointer to sequence and sets length, or nullptr if invalid
    const MidiEvent* getSequence(uint8_t tuneNumber, uint8_t quarter, uint16_t* outLength);
}

// Global instance
extern ClockChimes clockChimes;

#endif // CLOCKCHIMES_H
