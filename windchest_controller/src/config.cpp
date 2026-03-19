#include "config.h"
#include "logger.h"
#include <Preferences.h>

static Config     g_config;
static Preferences prefs;

// Default: MIDI note 34 → output 0, sequential up to num_outputs, all other notes ignored.
// Only MIDI channel 0 enabled by default.
static void set_defaults() {
    g_config.num_outputs = 56;  // 7 × 8 bits
    for (int ch = 0; ch < MIDI_CHANNELS; ch++) {
        g_config.midi[ch].enabled = (ch == 0);
        for (int n = 0; n < 128; n++) {
            int out = n - 34;
            g_config.midi[ch].note_to_output[n] =
                (out >= 0 && out < 56) ? (int8_t)out : -1;
        }
    }
}

void config_begin() {
    set_defaults();

    prefs.begin("windchest", /*readOnly=*/true);

    if (prefs.isKey("num_out")) {
        g_config.num_outputs = prefs.getUChar("num_out", 56);
    }

    for (int ch = 0; ch < MIDI_CHANNELS; ch++) {
        char key[12];
        snprintf(key, sizeof(key), "ch%d_en", ch);
        if (prefs.isKey(key)) {
            g_config.midi[ch].enabled = prefs.getBool(key, ch == 0);
        }
        snprintf(key, sizeof(key), "ch%d_map", ch);
        if (prefs.isKey(key)) {
            prefs.getBytes(key, g_config.midi[ch].note_to_output, 128);
        }
    }

    prefs.end();

    Log.printf("Config: num_outputs=%u, enabled MIDI channels:", g_config.num_outputs);
    for (int ch = 0; ch < MIDI_CHANNELS; ch++) {
        if (g_config.midi[ch].enabled) Log.printf(" %d", ch);
    }
    Log.println();
}

void config_save() {
    prefs.begin("windchest", /*readOnly=*/false);
    prefs.putUChar("num_out", g_config.num_outputs);
    for (int ch = 0; ch < MIDI_CHANNELS; ch++) {
        char key[12];
        snprintf(key, sizeof(key), "ch%d_en", ch);
        prefs.putBool(key, g_config.midi[ch].enabled);
        snprintf(key, sizeof(key), "ch%d_map", ch);
        prefs.putBytes(key, g_config.midi[ch].note_to_output, 128);
    }
    prefs.end();
    Log.println("Config: full save done");
}

void config_save_num_outputs() {
    prefs.begin("windchest", false);
    prefs.putUChar("num_out", g_config.num_outputs);
    prefs.end();
}

void config_save_channel(uint8_t ch) {
    if (ch >= MIDI_CHANNELS) return;
    prefs.begin("windchest", false);
    char key[12];
    snprintf(key, sizeof(key), "ch%d_en", ch);
    prefs.putBool(key, g_config.midi[ch].enabled);
    snprintf(key, sizeof(key), "ch%d_map", ch);
    prefs.putBytes(key, g_config.midi[ch].note_to_output, 128);
    prefs.end();
    Log.printf("Config: channel %d saved\n", ch);
}

Config& config_get() { return g_config; }

uint8_t config_num_outputs() { return g_config.num_outputs; }

bool config_channel_enabled(uint8_t ch) {
    return ch < MIDI_CHANNELS && g_config.midi[ch].enabled;
}

int config_note_to_output(uint8_t ch, uint8_t note) {
    if (ch >= MIDI_CHANNELS || note >= 128) return -1;
    return g_config.midi[ch].note_to_output[note];
}
