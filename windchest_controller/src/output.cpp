#include <Arduino.h>
#include <driver/gpio.h>   // ESP-IDF GPIO — available before Arduino init
#include "output.h"
#include "config.h"
#include "pins.h"
#include "logger.h"

static uint8_t outBuf[MAX_OUTPUT_BYTES];  // always 16 bytes; only the active slice is shifted out

// Runs as early as possible — before setup() — using ESP-IDF GPIO directly.
// Drives /OE HIGH (outputs disabled) so the 74HC595 indeterminate storage state
// never drives the load while the rest of the firmware initializes.
static void __attribute__((constructor)) output_early_disable() {
  gpio_set_direction((gpio_num_t)PIN_OE_N, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)PIN_OE_N, 1);   // HIGH = outputs disabled
}

void output_begin() {
  // Configure all shift-register control pins as outputs.
  // (output_early_disable already configured PIN_OE_N, but pinMode is harmless.)
  pinMode(PIN_CLR_N, OUTPUT);
  pinMode(PIN_OE_N,  OUTPUT);
  pinMode(PIN_SCK,   OUTPUT);
  pinMode(PIN_MOSI,  OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);

  // Keep outputs disabled while we set up.
  digitalWrite(PIN_OE_N, HIGH);   // /OE HIGH = outputs disabled (tri-state)

  // Pulse /SRCLR LOW to clear the shift registers.
  // Note: /SRCLR only clears the shift register, not the storage register —
  // we still need to clock in zeros and latch to fully clear the outputs.
  digitalWrite(PIN_CLR_N, LOW);
  delayMicroseconds(1);
  digitalWrite(PIN_CLR_N, HIGH);  // release — normal operation

  // Clock all-zero data into shift registers, then latch to storage registers.
  clearAll();
  flushOutput();

  // NOW enable outputs — storage registers are guaranteed all-zero.
  digitalWrite(PIN_OE_N, LOW);    // /OE LOW = outputs enabled
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
  int activeBytes = (config_num_outputs() + 7) / 8;
  shiftOutBytes(outBuf, activeBytes);
}

void clearAll() {
  memset(outBuf, 0x00, MAX_OUTPUT_BYTES);
}

void setAll(bool v) {
  memset(outBuf, v ? 0xFF : 0x00, MAX_OUTPUT_BYTES);
}

void stopAllNotes() {
  clearAll();
  flushOutput();
}

void setChannel(int idx, bool v) {
  if (idx < 0 || idx >= config_num_outputs()) return;
  Log.printf("setChannel(%d, %d)\n", idx, v ? 1 : 0);
  int byteIndex = idx / 8;
  int bitIndex  = idx % 8;
  if (v) outBuf[byteIndex] |=  (1 << bitIndex);
  else   outBuf[byteIndex] &= ~(1 << bitIndex);
}