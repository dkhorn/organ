#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Maximum buffer size (always 128 — more than any real windchest needs)
#define MAX_OUTPUT_CHANNELS 128
#define MIDI_CHANNELS       16

// Per-MIDI-channel mapping config
struct MidiChannelMap {
    bool   enabled;
    int8_t note_to_output[128];  // -1 = not mapped; 0..MAX_OUTPUT_CHANNELS-1 = output index
};

// Full runtime configuration (lives in RAM, backed by NVS)
struct Config {
    uint8_t        num_outputs;           // how many shift-register bits are active
    MidiChannelMap midi[MIDI_CHANNELS];   // per-channel note → output mapping
};

// ---------- Lifecycle ----------

/** Load from NVS (or apply defaults). Call once early in setup(), before output/midi inits. */
void config_begin();

/** Persist the entire config to NVS. */
void config_save();

/** Persist only num_outputs to NVS. */
void config_save_num_outputs();

/** Persist one MIDI channel's config to NVS. */
void config_save_channel(uint8_t ch);

// ---------- Accessors ----------

Config&  config_get();
uint8_t  config_num_outputs();
bool     config_channel_enabled(uint8_t midi_ch);

/** Returns output index (0-based), or -1 if this note should be ignored. */
int      config_note_to_output(uint8_t midi_ch, uint8_t note);

#endif // CONFIG_H
