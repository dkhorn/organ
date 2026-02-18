#ifndef MIDIRECEIVER_H
#define MIDIRECEIVER_H

#include <Arduino.h>

/**
 * MIDI receiver module - receives MIDI messages from hardware UART
 * 
 * Receives MIDI on GPIO38 at 31250 baud (MIDI standard)
 * Uses UART2 for MIDI input
 */
class MidiReceiver {
public:
    /**
     * Initialize MIDI receiver
     * Sets up UART2 on GPIO38 at 31250 baud
     */
    void begin();
    
    /**
     * Process incoming MIDI data - call from main loop
     * Reads and parses MIDI messages, logs them for now
     */
    void update();
    
private:
    // MIDI parser state
    enum class ParserState : uint8_t {
        IDLE,           // Waiting for status byte
        NOTE_OFF_1,     // Waiting for note number
        NOTE_OFF_2,     // Waiting for velocity
        NOTE_ON_1,      // Waiting for note number
        NOTE_ON_2,      // Waiting for velocity
        POLY_AT_1,      // Polyphonic aftertouch - note
        POLY_AT_2,      // Polyphonic aftertouch - pressure
        CC_1,           // Control change - controller number
        CC_2,           // Control change - value
        PROGRAM_1,      // Program change - program number
        CHANNEL_AT_1,   // Channel aftertouch - pressure
        PITCH_1,        // Pitch bend - LSB
        PITCH_2         // Pitch bend - MSB
    };
    
    ParserState state = ParserState::IDLE;
    uint8_t channel = 0;
    uint8_t data1 = 0;
    uint8_t data2 = 0;
    
    void handleMessage();
    void reset();
};

// Global instance
extern MidiReceiver midiReceiver;

#endif // MIDIRECEIVER_H
