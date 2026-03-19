#ifndef OUTPUT_H
#define OUTPUT_H

// Static buffer size — never needs to change even if num_outputs is reconfigured at runtime.
// The active output count is stored in config (see config.h / config_num_outputs()).
#define MAX_OUTPUT_CHANNELS 128
#define MAX_OUTPUT_BYTES    ((MAX_OUTPUT_CHANNELS + 7) / 8)  // 16 bytes


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