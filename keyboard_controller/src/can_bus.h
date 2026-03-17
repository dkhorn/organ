#ifndef CAN_BUS_H
#define CAN_BUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise and start the TWAI (CAN 2.0A) peripheral.
 * Call once from setup() before sending any messages.
 * Pins are taken from pins.h (PIN_CAN_TX / PIN_CAN_RX).
 * Baud rate: 500 kbit/s.
 */
void can_bus_begin();

/**
 * Transmit a Note On CAN frame.
 * CAN ID = (0x001 << 8) | channel  (per can-protocol.md)
 *
 * @param channel  CAN channel (0 = Great, 1 = Swell, …)
 * @param note     MIDI note number (0-127)
 * @param velocity Velocity (1-127)
 */
void can_send_note_on(uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * Transmit a Note Off CAN frame.
 * CAN ID = (0x000 << 8) | channel  (per can-protocol.md)
 *
 * @param channel  CAN channel (0 = Great, 1 = Swell, …)
 * @param note     MIDI note number (0-127)
 * @param velocity Release velocity (typically 64)
 */
void can_send_note_off(uint8_t channel, uint8_t note, uint8_t velocity);

#ifdef __cplusplus
}
#endif

#endif // CAN_BUS_H
