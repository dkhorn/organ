#ifndef MIDIHANDLER_H
#define MIDIHANDLER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Common MIDI message handler.
 * Dispatches MIDI messages to appropriate subsystems (midinote, etc.)
 * 
 * @param status Full MIDI status byte (0x80-0xEF)
 * @param data1 First data byte
 * @param data2 Second data byte (ignored for 2-byte messages)
 */
void handle_midi_message(uint8_t status, uint8_t data1, uint8_t data2);

#ifdef __cplusplus
}
#endif

#endif // MIDIHANDLER_H
