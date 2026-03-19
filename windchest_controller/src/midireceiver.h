#ifndef MIDIRECEIVER_H
#define MIDIRECEIVER_H

#include <Arduino.h>

/**
 * MIDI receiver module - receives MIDI messages from hardware UART.
 * Receives MIDI on PIN_MIDI_RX (see pins.h) at 31250 baud via Serial2.
 *
 * SIGNAL INVERSION: standard opto-isolator circuits (6N138 etc. with
 * collector pull-up) invert the signal.  If you receive garbage or nothing
 * at all, flip MIDI_RX_INVERT to true in midireceiver.cpp and re-flash.
 */
class MidiReceiver {
public:
    void begin();

    /** Call every loop iteration to process incoming bytes. */
    void update();

    // --- Diagnostic counters (read from /midi/diag) ---
    uint32_t bytesReceived  = 0;  // total raw bytes seen by UART
    uint8_t  lastByte       = 0;  // value of the most recent byte
    bool     pinSeenHigh    = false;  // GPIO ever read HIGH
    bool     pinSeenLow     = false;  // GPIO ever read LOW
    int      lastPinState   = -1;     // most recent digitalRead (-1 = not sampled yet)
    uint32_t messagesHandled = 0;     // fully-decoded note on/off messages dispatched

private:
    // MIDI parser state
    enum class ParserState : uint8_t {
        IDLE,
        NOTE_OFF_1, NOTE_OFF_2,
        NOTE_ON_1,  NOTE_ON_2,
        POLY_AT_1,  POLY_AT_2,
        CC_1,       CC_2,
        PROGRAM_1,
        CHANNEL_AT_1,
        PITCH_1,    PITCH_2
    };

    ParserState state   = ParserState::IDLE;
    uint8_t     channel = 0;
    uint8_t     data1   = 0;
    uint8_t     data2   = 0;

    void handleMessage();
    void reset();
};

// Global instance
extern MidiReceiver midiReceiver;

#endif // MIDIRECEIVER_H
