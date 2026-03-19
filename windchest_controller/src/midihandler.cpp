#include "midihandler.h"
#include "midinote.h"

extern "C" {

void handle_midi_message(uint8_t status, uint8_t data1, uint8_t data2) {
    uint8_t type    = status & 0xF0;
    uint8_t channel = status & 0x0F;

    switch (type) {
        case 0x80:  // Note Off
            note_off(channel, data1, data2);
            break;

        case 0x90:  // Note On (velocity 0 = note off)
            if (data2 == 0)
                note_off(channel, data1, 0);
            else
                note_on(channel, data1, data2);
            break;

        case 0xB0:  // Control Change
            if (data1 == 123) all_off();  // CC 123 = All Notes Off
            break;

        default:
            break;
    }
}

} // extern "C"
