#ifndef PINS_H
#define PINS_H
// ---------- Pin assignments ----------
const int PIN_MOSI   = 11;
const int PIN_SCK    = 12;
const int PIN_LATCH  = 10;
const int PIN_OE_N   = 9;
const int PIN_CLR_N  = 8;

const int PIN_BUTTON = 4;   // button to GND, use INPUT_PULLUP

// CAN bus (TWAI)
const int PIN_CAN_RX = 1;   // GPIO1, physical pin 41
const int PIN_CAN_TX = 2;   // GPIO2, physical pin 40

#endif