#include <Arduino.h>
#include "output.h"
#include "pins.h"
#include "logger.h"

const int  NUM_BYTES       = (NUM_CHANNELS + 7) / 8;   // 3 bytes

uint8_t outBuf[NUM_BYTES];

void output_begin() {
  // Initialize output buffer to all off
  clearAll();
  flushOutput();
}

// ---------- Helpers ----------
void shiftOutBytes(const uint8_t *buf, int len) {
  digitalWrite(PIN_LATCH, LOW);

  // buf[0] = channels 0..7, buf[1] = 8..15, buf[2] = 16..23
  // MSB-first inside each byte.
  for (int i = len - 1; i >= 0; --i) {
    uint8_t b = buf[i];
    for (int bit = 7; bit >= 0; --bit) {
      digitalWrite(PIN_SCK, LOW);
      digitalWrite(PIN_MOSI, (b & (1 << bit)) ? HIGH : LOW);
      digitalWrite(PIN_SCK, HIGH);
    }
  }

  digitalWrite(PIN_LATCH, HIGH);
}

void flushOutput() {
  shiftOutBytes(outBuf, NUM_BYTES);
}

void clearAll() {
  memset(outBuf, 0x00, NUM_BYTES);
}

void setAll(bool v) {
  memset(outBuf, v ? 0xFF : 0x00, NUM_BYTES);
}

void stopAllNotes() {
  clearAll();
  flushOutput();
}

void setChannel(int idx, bool v) {
  if (idx < 0 || idx >= NUM_CHANNELS) return;
  Log.printf("setChannel(%d, %d)\n", idx, v ? 1 : 0);
  int byteIndex = idx / 8;
  int bitIndex  = idx % 8;   // channel 0 = bit0 of buf[0]
  if (v) outBuf[byteIndex] |=  (1 << bitIndex);
  else  outBuf[byteIndex] &= ~(1 << bitIndex);
}