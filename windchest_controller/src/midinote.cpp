// midinote.cpp
#include <Arduino.h>
#include "midinote.h"
#include "output.h"
#include "logger.h"

// #include "chimes.h"
// #include "noterepeater.h"

// MIDI note tracking - which notes are currently "on"
static bool note_state[128] = {false};

static int midinote_to_outputindex(uint8_t midi_note) {
  return midi_note - 72;
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
  int output_index = midinote_to_outputindex(midi_note);
  
  if (output_index >= 0) {
    // Mark note as on
    note_state[midi_note] = true;

    Log.printf("MIDI Note On: %d -> index: %d\n", midi_note, output_index);
    // Start the note
    setChannel(output_index, true);
    flushOutput();
    
    // Serial.printf("MIDI Note On: %d -> Chime: %d\n", midi_note, chime_note);
  } else {
    // Serial.printf("MIDI Note On: %d -> Out of range\n", midi_note);
  }
}

void note_off(uint8_t midi_note, uint8_t velocity) {
  // Ignore velocity for now
  (void)velocity;
  
  if (midi_note >= 128) return;
  
  // Mark note as off
  note_state[midi_note] = false;
  
  int output_index = midinote_to_outputindex(midi_note);

  Log.printf("MIDI Note Off: %d -> index: %d\n", midi_note, output_index);

  setChannel(output_index, false);
  flushOutput();
  // Serial.printf("MIDI Note Off: %d\n", midi_note);
}

void all_off() {
  // Turn off all MIDI note states
  for (int i = 0; i < 128; i++) {
    note_state[i] = false;
  }
  
  // Reset all chime plungers to idle
  stopAllNotes();
  
  // Stop all repeating notes
  // stop_all_repeated_notes();
  
  // Serial.println("All notes off");
}

} // extern "C"
