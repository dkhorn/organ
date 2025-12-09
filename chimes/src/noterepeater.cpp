#include "noterepeater.h"
#include "chimes.h"
#include "midinote.h"
#include "logger.h"

// Maximum number of simultaneously repeating notes
#define MAX_REPEATING_NOTES 20

struct RepeatingNote {
  uint8_t note;           // MIDI note number
  uint8_t velocity;       // Strike velocity
  uint32_t period_ms;     // Time between strikes
  uint16_t repeat_count;  // Remaining repeats (0 = forever)
  uint32_t next_strike;   // millis() time of next strike
  bool active;            // Is this slot active?
};

static RepeatingNote repeating_notes[MAX_REPEATING_NOTES];

void noterepeater_setup() {
  // Initialize all slots as inactive
  for (int i = 0; i < MAX_REPEATING_NOTES; i++) {
    repeating_notes[i].active = false;
  }
  Log.println("Note repeater initialized");
}

void noterepeater_loop() {
  uint32_t now = millis();
  
  for (int i = 0; i < MAX_REPEATING_NOTES; i++) {
    if (!repeating_notes[i].active) continue;
    
    // Check if it's time to strike
    if (now >= repeating_notes[i].next_strike) {
      // Strike the note (note_on followed immediately by note_off)
      note_on(repeating_notes[i].note, repeating_notes[i].velocity);
      note_off(repeating_notes[i].note, 64);
      Log.printf("Repeater: Struck note %d (vel=%d, period=%dms, count=%d)\n",
                 repeating_notes[i].note, repeating_notes[i].velocity, 
                 repeating_notes[i].period_ms, repeating_notes[i].repeat_count);
      
      // Schedule next strike
      repeating_notes[i].next_strike = now + repeating_notes[i].period_ms;
      
      // Decrement repeat count if not infinite
      if (repeating_notes[i].repeat_count > 0) {
        repeating_notes[i].repeat_count--;
        if (repeating_notes[i].repeat_count == 0) {
          // Done repeating
          repeating_notes[i].active = false;
          Log.printf("Repeater: Note %d finished (count expired)\n", repeating_notes[i].note);
        }
      }
    }
  }
}

void start_repeated_note(uint8_t note, uint8_t velocity, uint32_t period_ms, uint16_t repeat_count) {
  Log.printf("Repeater: Start note=%d vel=%d period=%dms count=%d\n", 
             note, velocity, period_ms, repeat_count);
  
  // First check if this note is already repeating and update it
  for (int i = 0; i < MAX_REPEATING_NOTES; i++) {
    if (repeating_notes[i].active && repeating_notes[i].note == note) {
      // Update existing repeater
      repeating_notes[i].velocity = velocity;
      repeating_notes[i].period_ms = period_ms;
      repeating_notes[i].repeat_count = repeat_count;
      repeating_notes[i].next_strike = millis(); // Strike immediately
      Log.printf("Repeater: Updated existing slot %d for note %d\n", i, note);
      return;
    }
  }
  
  // Find an empty slot
  for (int i = 0; i < MAX_REPEATING_NOTES; i++) {
    if (!repeating_notes[i].active) {
      repeating_notes[i].note = note;
      repeating_notes[i].velocity = velocity;
      repeating_notes[i].period_ms = period_ms;
      repeating_notes[i].repeat_count = repeat_count;
      repeating_notes[i].next_strike = millis(); // Strike immediately
      repeating_notes[i].active = true;
      Log.printf("Repeater: Added to slot %d for note %d\n", i, note);
      return;
    }
  }
  
  // No slots available - could log an error here
  Log.printf("Repeater: ERROR - No slots available for note %d\n", note);
}

void stop_repeated_note(uint8_t note) {
  for (int i = 0; i < MAX_REPEATING_NOTES; i++) {
    if (repeating_notes[i].active && repeating_notes[i].note == note) {
      repeating_notes[i].active = false;
      Log.printf("Repeater: Stopped note %d (slot %d)\n", note, i);
      return;
    }
  }
  Log.printf("Repeater: Note %d was not repeating\n", note);
}

void stop_all_repeated_notes() {
  int stopped = 0;
  for (int i = 0; i < MAX_REPEATING_NOTES; i++) {
    if (repeating_notes[i].active) {
      stopped++;
    }
    repeating_notes[i].active = false;
  }
  Log.printf("Repeater: Stopped all (%d active notes)\n", stopped);
}
