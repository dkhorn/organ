#pragma once

#include <Arduino.h>

// Initialize the note repeater module
void noterepeater_setup();

// Update the note repeater (call from main loop)
void noterepeater_loop();

// Start repeating a note at a given period
// note: MIDI note number (69-88)
// velocity: MIDI velocity (1-127)
// period_ms: Time between strikes in milliseconds
// repeat_count: Number of times to repeat (0 = forever)
void start_repeated_note(uint8_t note, uint8_t velocity, uint32_t period_ms, uint16_t repeat_count);

// Stop repeating a specific note
void stop_repeated_note(uint8_t note);

// Stop all repeating notes
void stop_all_repeated_notes();
