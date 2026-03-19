#include "can_bus.h"
#include "pins.h"
#include "logger.h"

#include "driver/twai.h"

// CAN channel number for "Great" manual (per can-protocol.md)
#define CAN_CHANNEL_GREAT 0

extern "C" {

void can_bus_begin() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)PIN_CAN_TX,
        (gpio_num_t)PIN_CAN_RX,
        TWAI_MODE_NORMAL
    );
    // Small TX queue is enough for proof-of-concept; increase if needed.
    g_config.tx_queue_len = 8;
    g_config.rx_queue_len = 8;

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Log.printf("CAN: driver install failed (%d)\n", err);
        return;
    }

    err = twai_start();
    if (err != ESP_OK) {
        Log.printf("CAN: start failed (%d)\n", err);
        return;
    }

    Log.println("CAN: started at 500 kbit/s (TX=GPIO2, RX=GPIO1)");
}

// Internal helper — builds and transmits a standard 11-bit CAN frame.
static void can_send(uint32_t can_id, uint8_t note, uint8_t velocity) {
    twai_message_t msg = {};
    msg.extd            = 0;   // Standard frame (11-bit ID)
    msg.rtr             = 0;   // Data frame
    msg.identifier      = can_id;
    msg.data_length_code = 3;
    msg.data[0]         = note;
    msg.data[1]         = velocity;
    msg.data[2]         = 0x00;

    esp_err_t err = twai_transmit(&msg, pdMS_TO_TICKS(10));
    if (err != ESP_OK) {
        Log.printf("CAN: tx failed (id=0x%03" PRIX32 ", err=%d)\n", can_id, err);
    }
}

void can_send_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Note On  →  msg_type = 0b001  →  CAN_ID = (1 << 8) | channel
    uint32_t id = (0x001u << 8) | channel;
    Log.printf("CAN tx: Note On  ch=%u note=%u vel=%u (id=0x%03" PRIX32 ")\n",
               channel, note, velocity, id);
    can_send(id, note, velocity);
}

void can_send_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Note Off →  msg_type = 0b000  →  CAN_ID = (0 << 8) | channel
    uint32_t id = (0x000u << 8) | channel;
    Log.printf("CAN tx: Note Off ch=%u note=%u vel=%u (id=0x%03" PRIX32 ")\n",
               channel, note, velocity, id);
    can_send(id, note, velocity);
}

} // extern "C"
