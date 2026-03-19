// midinote.cpp
#include <Arduino.h>
#include "midinote.h"
#include "config.h"
#include "output.h"
#include "logger.h"

extern "C" {

void midinote_begin() {
  // nothing to initialise — state lives in outBuf (output.cpp)
}

void note_on(uint8_t midi_ch, uint8_t midi_note, uint8_t velocity) {
  (void)velocity;
  if (!config_channel_enabled(midi_ch)) return;
  int out = config_note_to_output(midi_ch, midi_note);
  if (out < 0) return;
  Log.printf("Note On:  ch%u note%u -> out%d\n", midi_ch, midi_note, out);
  setChannel(out, true);
  flushOutput();
}

void note_off(uint8_t midi_ch, uint8_t midi_note, uint8_t velocity) {
  (void)velocity;
  if (!config_channel_enabled(midi_ch)) return;
  int out = config_note_to_output(midi_ch, midi_note);
  if (out < 0) return;
  Log.printf("Note Off: ch%u note%u -> out%d\n", midi_ch, midi_note, out);
  setChannel(out, false);
  flushOutput();
}

void all_off() {
  stopAllNotes();
}

} // extern "C"
