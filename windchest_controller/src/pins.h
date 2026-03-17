#ifndef PINS_H
#define PINS_H
// ---------- Pin assignments (ESP32-S3-DevKitC-1) ----------

// SPI shift-register bus
const int PIN_CLR_N  = 10;  // /SRCLR  - GPIO10, physical pin 16
const int PIN_OE_N   = 11;  // /OE     - GPIO11, physical pin 17
const int PIN_SCK    = 12;  // SCK     - GPIO12, physical pin 18
const int PIN_MOSI   = 13;  // MOSI    - GPIO13, physical pin 19
const int PIN_LATCH  = 14;  // LATCH   - GPIO14, physical pin 20

// MIDI serial input (UART2)
const int PIN_MIDI_RX = 17; // GPIO17, physical pin 10

#endif