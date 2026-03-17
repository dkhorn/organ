#ifndef PINS_H
#define PINS_H

// ---------- I2C bus (MCP23017 key scanner) ----------
const int PIN_I2C_SDA = 4;   // GPIO4  – SDA (4.7 kΩ pull-up fitted)
const int PIN_I2C_SCL = 5;   // GPIO5  – SCL (4.7 kΩ pull-up fitted)
const int PIN_I2C_INT = 6;   // GPIO6  – shared active-low INT from all MCP23017s (4.7 kΩ pull-up fitted)

// ---------- CAN bus (TWAI) ----------
const int PIN_CAN_RX = 1;    // GPIO1, physical pin 41
const int PIN_CAN_TX = 2;    // GPIO2, physical pin 40

// Keyboard scanner parameters (base note and CAN channel) are now determined
// at runtime by the hardware_id stored in NVS.  See key_scanner_get_hardware_id()
// and key_scanner_get_can_channel().

#endif