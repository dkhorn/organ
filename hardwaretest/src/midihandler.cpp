#include "midihandler.h"
#include "midinote.h"

extern "C" {

void handle_midi_message(uint8_t status, uint8_t data1, uint8_t data2) {
    uint8_t type = status & 0xF0;
    uint8_t channel = status & 0x0F;
    
    (void)channel;  // Not used yet, but available for future channel filtering
    
    switch (type) {
        case 0x80:  // Note Off
            note_off(data1, data2);
            break;
            
        case 0x90:  // Note On
            if (data2 == 0) {
                // Velocity 0 = Note Off
                note_off(data1, 0);
            } else {
                note_on(data1, data2);
            }
            break;
            
        case 0xA0:  // Polyphonic Aftertouch
            // Not implemented yet
            break;
            
        case 0xB0:  // Control Change
            // Special case: CC 123 (All Notes Off)
            if (data1 == 123) {
                all_off();
            }
            // Other CCs not implemented yet
            break;
            
        case 0xC0:  // Program Change
            // Not implemented yet
            break;
            
        case 0xD0:  // Channel Pressure
            // Not implemented yet
            break;
            
        case 0xE0:  // Pitch Bend
            // Not implemented yet
            break;
            
        default:
            // Invalid or unsupported message type
            break;
    }
}

} // extern "C"
