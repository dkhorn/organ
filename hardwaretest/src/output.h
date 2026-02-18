#ifndef OUTPUT_H
#define OUTPUT_H

#define NUM_CHANNELS    12


#include <stdint.h>
#include <stddef.h>  // For size_t


#ifdef __cplusplus
extern "C" {
#endif

void flushOutput();

void clearAll();
  
void setAll(bool v);
  
void setChannel(int idx, bool v);

void stopAllNotes();

void output_begin();

#ifdef __cplusplus
}
#endif

#endif