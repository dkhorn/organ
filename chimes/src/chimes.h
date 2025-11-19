#ifndef CHIMES_H
#define CHIMES_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the chimes system
void chimes_begin(void);

// Cleanup the chimes system
void chimes_loop(void);

// Play a chime sound
void ring_chime(int ch);

#ifdef __cplusplus
}
#endif

#endif // CHIMES_H