#ifndef CHIMES_H
#define CHIMES_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the chimes system
void chimes_begin(void);

// Update chimes state machine (call frequently in main loop)
void chimes_loop(void);

// Ring a chime by note number (0-20)
// Note number is mapped to physical channel via NOTE_TO_CHANNEL array
void ring_chime(int note, int velocity);

void ring_chime_raw(int ch, int dutyPct, int kickHoldTimeMs);

// Ring a chime by physical channel number (0-20), bypassing note mapping
void ring_chime_by_channel(int ch, int velocity);

// Reset all chime plungers to idle state
void chimes_all_off(void);

#ifdef __cplusplus
}
#endif

#endif // CHIMES_H