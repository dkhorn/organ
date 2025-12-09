// midiseq.h - Simple non-blocking MIDI sequencer
#ifndef MIDISEQ_H
#define MIDISEQ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// MIDI event structure
typedef struct {
  uint32_t delta_ticks;  // Ticks since previous event (MIDI delta time)
  uint8_t status;        // MIDI status byte (0x80-0xFF)
  uint8_t data1;         // First data byte (note number for note on/off)
  uint8_t data2;         // Second data byte (velocity for note on/off)
} MidiEvent;

// Initialize the sequencer
void midiseq_begin();

// Load a sequence (takes ownership of the event array)
// events: array of MIDI events
// num_events: number of events in the array
// ticks_per_quarter: MIDI ticks per quarter note (typically 480 or 96)
// tempo_bpm: playback tempo in beats per minute
// transpose_semitones: number of half-steps to transpose (default 0)
// max_velocity: maximum velocity (127 = no scaling, lower values scale proportionally)
void midiseq_load(const MidiEvent* events, uint16_t num_events, 
                  uint16_t ticks_per_quarter, uint16_t tempo_bpm,
                  int8_t transpose_semitones = 0, uint8_t max_velocity = 127);

// Start playback
void midiseq_play();

// Stop playback
void midiseq_stop();

// Pause/resume playback
void midiseq_pause();
void midiseq_resume();

// Check if playing
bool midiseq_is_playing();

// Update sequencer (call from main loop)
void midiseq_loop();

// Set tempo during playback
void midiseq_set_tempo(uint16_t tempo_bpm);

#ifdef __cplusplus
}
#endif

#endif // MIDISEQ_H
