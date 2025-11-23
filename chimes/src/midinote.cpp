// midinote.cpp
#include <Arduino.h>
#include "midinote.h"
#include "chimes.h"

// MIDI note tracking - which notes are currently "on"
static bool note_state[128] = {false};

// MIDI note number for A440 (MIDI note 69)
static const uint8_t MIDI_A440 = 69;

// Chime note number for A440 (chime #1, not #0)
static const uint8_t CHIME_A440 = 1;

// Convert MIDI note number to chime note number
// Returns -1 if note is out of range for our 21 chimes
static int midi_to_chime(uint8_t midi_note) {
  // Calculate offset from A440
  int offset = (int)midi_note - (int)MIDI_A440;
  
  // Apply offset to chime A440 position
  int chime_note = CHIME_A440 + offset;
  
  // Check if within valid range (0-20)
  if (chime_note < 0 || chime_note >= 21) {
    return -1;
  }
  
  return chime_note;
}

extern "C" {

void midinote_begin() {
  // Clear all note states
  for (int i = 0; i < 128; i++) {
    note_state[i] = false;
  }
}

void note_on(uint8_t midi_note, uint8_t velocity) {
  // Ignore velocity for now
  (void)velocity;
  
  if (midi_note >= 128) return;
  
  // Convert MIDI note to chime note
  int chime_note = midi_to_chime(midi_note);
  
  if (chime_note >= 0) {
    // Mark note as on
    note_state[midi_note] = true;
    
    // Ring the chime
    ring_chime(chime_note);
    
    Serial.printf("MIDI Note On: %d -> Chime: %d\n", midi_note, chime_note);
  } else {
    Serial.printf("MIDI Note On: %d -> Out of range\n", midi_note);
  }
}

void note_off(uint8_t midi_note, uint8_t velocity) {
  // Ignore velocity for now
  (void)velocity;
  
  if (midi_note >= 128) return;
  
  // Mark note as off
  note_state[midi_note] = false;
  
  // Nothing else to do until we add dampers
  Serial.printf("MIDI Note Off: %d\n", midi_note);
}

void all_off() {
  // Turn off all MIDI note states
  for (int i = 0; i < 128; i++) {
    note_state[i] = false;
  }
  
  // Reset all chime plungers to idle
  chimes_all_off();
  
  Serial.println("All notes off");
}

} // extern "C"
