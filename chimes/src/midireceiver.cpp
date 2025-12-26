#include "midireceiver.h"
#include "logger.h"
#include "midihandler.h"

// MIDI uses UART at 31250 baud, 8-N-1
#define MIDI_BAUD_RATE 31250
#define MIDI_RX_PIN 44
#define MIDI_UART_NUM 2

// Global instance
MidiReceiver midiReceiver;

void MidiReceiver::begin() {
    Log.println("Initializing MIDI receiver...");
    
    // pinMode(MIDI_RX_PIN, INPUT);
    // Initialize UART2 for MIDI input
    // MIDI: 31250 baud, 8 data bits, no parity, 1 stop bit
    // RX only (no TX needed for MIDI input)
    Serial.begin(MIDI_BAUD_RATE); // , SERIAL_8N1, MIDI_RX_PIN, -1);
    
    // Initialize parser state
    reset();
    
    Log.printf("MIDI receiver on GPIO%d at %d baud\n", MIDI_RX_PIN, MIDI_BAUD_RATE);
}

// DEBUG: Sample GPIO38 as digital input to check for activity
static void debug_sample_gpio38() {
    static unsigned long lastSample = 0;
    unsigned long now = millis();
    if (now - lastSample >= 100) { // every 10ms
        lastSample = now;
        int val = digitalRead(MIDI_RX_PIN);
        Log.printf("%d ", val);
    }
}

void MidiReceiver::update() {
    static unsigned long lastByteTime = 0;
    unsigned long now = millis();
    // Timeout: reset parser if no bytes for 100ms
    if (now - lastByteTime > 100) {
        reset();
    }
    // debug_sample_gpio38(); // DEBUG: sample GPIO38 as input
    // Process all available MIDI bytes
    while (Serial.available()) {
        uint8_t byte = Serial.read();
        lastByteTime = millis();
        // Log.printf("MIDI RAW: 0x%02X\n", byte); // DEBUG: log every byte
        
        // Check for status byte (bit 7 set)
        if (byte & 0x80) {
            // Status byte received
            uint8_t statusNibble = (byte >> 4) & 0x07;  // Upper nibble (bits 4-6)
            channel = byte & 0x0F;                       // Lower nibble (bits 0-3)
            
            switch (statusNibble) {
                case 0: // Note Off (0x80-0x8F)
                    state = ParserState::NOTE_OFF_1;
                    break;
                    
                case 1: // Note On (0x90-0x9F)
                    state = ParserState::NOTE_ON_1;
                    break;
                    
                case 2: // Polyphonic Aftertouch (0xA0-0xAF)
                    state = ParserState::POLY_AT_1;
                    break;
                    
                case 3: // Control Change (0xB0-0xBF)
                    state = ParserState::CC_1;
                    break;
                    
                case 4: // Program Change (0xC0-0xCF)
                    state = ParserState::PROGRAM_1;
                    break;
                    
                case 5: // Channel Aftertouch (0xD0-0xDF)
                    state = ParserState::CHANNEL_AT_1;
                    break;
                    
                case 6: // Pitch Bend (0xE0-0xEF)
                    state = ParserState::PITCH_1;
                    break;
                    
                case 7: // System messages (0xF0-0xFF)
                    // Reset parser on system messages for now
                    // Log.printf("MIDI: System message 0x%02X\n", byte);
                    reset();
                    break;
            }
        } else {
            // Data byte (bit 7 clear)
            switch (state) {
                case ParserState::NOTE_OFF_1:
                    data1 = byte;
                    state = ParserState::NOTE_OFF_2;
                    break;
                    
                case ParserState::NOTE_OFF_2:
                    data2 = byte;
                    handleMessage();
                    state = ParserState::NOTE_OFF_1;  // Ready for running status
                    break;
                    
                case ParserState::NOTE_ON_1:
                    data1 = byte;
                    state = ParserState::NOTE_ON_2;
                    break;
                    
                case ParserState::NOTE_ON_2:
                    data2 = byte;
                    handleMessage();
                    state = ParserState::NOTE_ON_1;  // Ready for running status
                    break;
                    
                case ParserState::POLY_AT_1:
                    data1 = byte;
                    state = ParserState::POLY_AT_2;
                    break;
                    
                case ParserState::POLY_AT_2:
                    data2 = byte;
                    handleMessage();
                    state = ParserState::POLY_AT_1;  // Ready for running status
                    break;
                    
                case ParserState::CC_1:
                    data1 = byte;
                    state = ParserState::CC_2;
                    break;
                    
                case ParserState::CC_2:
                    data2 = byte;
                    handleMessage();
                    state = ParserState::CC_1;  // Ready for running status
                    break;
                    
                case ParserState::PROGRAM_1:
                    data1 = byte;
                    handleMessage();
                    state = ParserState::PROGRAM_1;  // Ready for running status
                    break;
                    
                case ParserState::CHANNEL_AT_1:
                    data1 = byte;
                    handleMessage();
                    state = ParserState::CHANNEL_AT_1;  // Ready for running status
                    break;
                    
                case ParserState::PITCH_1:
                    data1 = byte;
                    state = ParserState::PITCH_2;
                    break;
                    
                case ParserState::PITCH_2:
                    data2 = byte;
                    handleMessage();
                    state = ParserState::PITCH_1;  // Ready for running status
                    break;
                    
                case ParserState::IDLE:
                    // Received data byte without status - ignore
                    break;
            }
        }
    }
}

void MidiReceiver::handleMessage() {
    // Reconstruct status byte from current parser state
    uint8_t status = 0;
    
    switch (state) {
        case ParserState::NOTE_OFF_2:
            status = 0x80 | channel;
            break;
        case ParserState::NOTE_ON_2:
            status = 0x90 | channel;
            break;
        case ParserState::POLY_AT_2:
            status = 0xA0 | channel;
            break;
        case ParserState::CC_2:
            status = 0xB0 | channel;
            break;
        case ParserState::PROGRAM_1:
            status = 0xC0 | channel;
            break;
        case ParserState::CHANNEL_AT_1:
            status = 0xD0 | channel;
            break;
        case ParserState::PITCH_2:
            status = 0xE0 | channel;
            break;
        default:
            return;  // Unknown state
    }
    
    // Delegate to common MIDI handler
    handle_midi_message(status, data1, data2);
}

void MidiReceiver::reset() {
    state = ParserState::IDLE;
    channel = 0;
    data1 = 0;
    data2 = 0;
}
