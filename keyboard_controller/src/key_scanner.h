#ifndef KEY_SCANNER_H
#define KEY_SCANNER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the MCP23017 key scanner.
 *
 * Probes I2C addresses 0x20–0x27, configures every found chip as
 * all-inputs with internal pull-ups and interrupt-on-change (both edges),
 * then takes an initial snapshot of key states.
 *
 * Call once from setup() after Wire / CAN have been initialised.
 */
void key_scanner_begin();

/**
 * Process any pending key events.
 *
 * Must be called from loop().  When the shared INT line asserts (active-low),
 * every chip is polled; a CAN Note-On or Note-Off is sent for each changed key.
 * Keys are active-low: pin LOW = pressed → Note On, pin HIGH = released → Note Off.
 */
void key_scanner_update();

/**
 * Returns the number of MCP23017 chips discovered during begin().
 */
int key_scanner_chip_count();

/**
 * Get/set the hardware identity (0–5).  Persisted in NVS.
 *   0=Pedal  1=Great  2=Swell  3=Positiv  4=Echo  5=Stop-board
 * The hardware_id determines both the CAN output channel and the keymap.
 */
int     key_scanner_get_hardware_id();
void    key_scanner_set_hardware_id(int id);
uint8_t key_scanner_get_can_channel();

#ifdef __cplusplus
}
#endif

#endif // KEY_SCANNER_H
