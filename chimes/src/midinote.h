#ifndef MIDINOTE_H
#define MIDINOTE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize MIDI note system
void midinote_begin(void);

// Handle MIDI note on (note: 0-127, velocity: 0-127)
void note_on(uint8_t midi_note, uint8_t velocity);

// Handle MIDI note off (note: 0-127, velocity: 0-127)
void note_off(uint8_t midi_note, uint8_t velocity);

// Turn off all notes and reset all chimes
void all_off(void);

#ifdef __cplusplus
}
#endif

#endif // MIDINOTE_H
