// midiseq.cpp - Simple non-blocking MIDI sequencer
#include "midiseq.h"
#include "midinote.h"
#include <Arduino.h>

static const MidiEvent* sequence = nullptr;
static uint16_t num_events = 0;
static uint16_t current_event = 0;
static uint32_t ticks_per_quarter = 480;
static uint32_t microsec_per_tick = 0;
static uint32_t next_event_time = 0;
static bool playing = false;
static bool paused = false;
static int8_t transpose = 0;  // Transpose in semitones (half-steps)

// Calculate microseconds per tick based on tempo
static void calculate_timing(uint16_t tempo_bpm) {
  // MIDI timing: microseconds per quarter note = 60,000,000 / BPM
  uint32_t usec_per_quarter = 60000000UL / tempo_bpm;
  microsec_per_tick = usec_per_quarter / ticks_per_quarter;
}

void midiseq_begin() {
  sequence = nullptr;
  num_events = 0;
  current_event = 0;
  playing = false;
  paused = false;
}

void midiseq_load(const MidiEvent* events, uint16_t count, 
                  uint16_t tpq, uint16_t tempo_bpm, int8_t transpose_semitones) {
  // Stop current playback
  midiseq_stop();
  
  // Load new sequence
  sequence = events;
  num_events = count;
  current_event = 0;
  ticks_per_quarter = tpq;
  transpose = transpose_semitones;
  
  calculate_timing(tempo_bpm);
}

void midiseq_play() {
  if (!sequence || num_events == 0) return;
  
  current_event = 0;
  next_event_time = micros();
  playing = true;
  paused = false;
  
  Serial.printf("Starting MIDI playback: %d events, %d us/tick\n", 
                num_events, microsec_per_tick);
}

void midiseq_stop() {
  playing = false;
  paused = false;
  current_event = 0;
  
  // Send all notes off
  all_off();
}

void midiseq_pause() {
  paused = true;
}

void midiseq_resume() {
  if (playing && paused) {
    paused = false;
    // Reset timing to avoid jump
    next_event_time = micros();
  }
}

bool midiseq_is_playing() {
  return playing && !paused;
}

void midiseq_set_tempo(uint16_t tempo_bpm) {
  calculate_timing(tempo_bpm);
  Serial.printf("Tempo changed to %d BPM (%d us/tick)\n", 
                tempo_bpm, microsec_per_tick);
}

void midiseq_loop() {
  if (!playing || paused || !sequence) return;
  
  uint32_t now = micros();
  
  // Process all events that are due
  while (current_event < num_events && (int32_t)(now - next_event_time) >= 0) {
    const MidiEvent* evt = &sequence[current_event];
    
    // Handle MIDI event
    uint8_t status = evt->status & 0xF0;  // Upper nibble is message type
    uint8_t channel = evt->status & 0x0F; // Lower nibble is channel (unused for now)
    
    switch (status) {
      case 0x90: // Note On
        if (evt->data2 > 0) {  // Velocity > 0 means note on
          int16_t transposed_note = (int16_t)evt->data1 + transpose;
          if (transposed_note >= 0 && transposed_note <= 127) {
            note_on(transposed_note, evt->data2);
          }
          // Serial.printf("Note ON: %d vel=%d\n", evt->data1, evt->data2);
        } else {  // Velocity = 0 is actually note off
          int16_t transposed_note = (int16_t)evt->data1 + transpose;
          if (transposed_note >= 0 && transposed_note <= 127) {
            note_off(transposed_note, evt->data2);
          }
          // Serial.printf("Note OFF: %d\n", evt->data1);
        }
        break;
        
      case 0x80: // Note Off
        {
          int16_t transposed_note = (int16_t)evt->data1 + transpose;
          if (transposed_note >= 0 && transposed_note <= 127) {
            note_off(transposed_note, evt->data2);
          }
        }
        // Serial.printf("Note OFF: %d\n", evt->data1);
        break;
        
      // Other MIDI messages could be handled here:
      // 0xA0 = Polyphonic aftertouch
      // 0xB0 = Control change
      // 0xC0 = Program change
      // 0xD0 = Channel aftertouch
      // 0xE0 = Pitch bend
      
      default:
        // Ignore unknown messages
        break;
    }
    
    // Advance to next event
    current_event++;
    
    // Calculate next event time
    if (current_event < num_events) {
      uint32_t delta_usec = sequence[current_event].delta_ticks * microsec_per_tick;
      next_event_time += delta_usec;
    }
  }
  
  // Check if sequence finished
  if (current_event >= num_events) {
    Serial.println("MIDI sequence finished");
    midiseq_stop();
  }
}
