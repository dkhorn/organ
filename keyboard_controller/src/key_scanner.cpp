// key_scanner.cpp
// Scans organ keyboard keys via MCP23017 I/O expanders on the I2C bus.
//   Key pressed  (pin LOW)  → CAN Note On
//   Key released (pin HIGH) → CAN Note Off

#include <Arduino.h>
#include <Wire.h>
#include "key_scanner.h"
#include "pins.h"
#include "can_bus.h"
#include "logger.h"
#include <Preferences.h>

// ---- MCP23017 I2C addressing ----
#define MCP23017_BASE_ADDR  0x20   // Lowest address (A2=A1=A0=0)
#define MAX_CHIPS           8      // Addresses 0x20–0x27
#define KEYS_PER_CHIP       16     // 8 on port A + 8 on port B

// ---- MCP23017 register map (IOCON.BANK = 0, factory default) ----
#define REG_IODIRA    0x00   // I/O direction A        (1 = input)
#define REG_IODIRB    0x01   // I/O direction B
#define REG_GPINTENA  0x04   // Interrupt-on-change enable A
#define REG_GPINTENB  0x05   // Interrupt-on-change enable B
#define REG_INTCONA   0x08   // Interrupt control A    (0 = compare to previous state)
#define REG_INTCONB   0x09   // Interrupt control B
#define REG_IOCONA    0x0A   // I/O configuration register
#define REG_GPPUA     0x0C   // Pull-up resistor control A
#define REG_GPPUB     0x0D   // Pull-up resistor control B
#define REG_GPIOA     0x12   // GPIO port A (read clears interrupt)
#define REG_GPIOB     0x13   // GPIO port B

// IOCON bit mask:
//   bit 6  MIRROR – mirrors INTA/INTB so either port change asserts both outputs
//   bit 2  ODR    – open-drain INT output (required when multiple chips share one wire)
//   INTPOL (bit 1) = 0 → active-low (matches the pull-up to Vcc wiring)
#define IOCON_VALUE   0x44   // MIRROR=1, ODR=1, INTPOL=0

// ---- ISR flag (set on FALLING edge of shared INT line) ----
static volatile bool intFlag = false;

static void IRAM_ATTR onInterrupt() {
    intFlag = true;
}

// ---- Hardware identity ----
// Loaded from NVS at boot.  Determines the CAN output channel and keymap.
//   0 = Pedal   1 = Great   2 = Swell   3 = Positiv   4 = Echo   5 = Stop-board
static int hardwareId = 0;

// Expected MCP23017 chip count per hardware_id.
// Checked at boot; a mismatch is logged as a warning but does not halt operation.
//   Pedal:      3 chips  (0x23 + 0x25 = 32 keys; 0x27 reserved for future stops)
//   Manuals:    4 chips  (64 inputs; enough for 61-note manual + spares)
//   Stop-board: 6 chips  (96 inputs; exact count TBD when hardware is finalised)
static const int expectedChips[6] = {
    3,   // 0 = Pedal
    4,   // 1 = Great
    4,   // 2 = Swell
    4,   // 3 = Positiv
    4,   // 4 = Echo
    6,   // 5 = Stop-board
};

// ---- Key maps ----
// keymaps[hardware_id][chip_index * KEYS_PER_CHIP + pin] = MIDI note number.
// hardware_id 0 (Pedal) is fully calibrated; others use sequential defaults.
static const uint8_t keymaps[6][MAX_CHIPS * KEYS_PER_CHIP] = {
    // --- id 0: Pedal (calibrated) ---
    {
        // chip 0 (0x23), pins 0–15  →  MIDI 36–51
        44, 49, 38, 46, 47, 45, 40, 41, 51, 50, 39, 36, 37, 42, 43, 48,
        // chip 1 (0x25), pins 0–15  →  MIDI 52–67
        66, 55, 65, 64, 59, 57, 63, 62, 60, 61, 53, 52, 67, 58, 56, 54,
        // chip 2 (0x27), pins 0–15  →  reserved
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    },
    // --- id 1: Great (sequential default, uncalibrated) ---
    {36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
     52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // --- id 2: Swell (sequential default, uncalibrated) ---
    {36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
     52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // --- id 3: Positiv (sequential default, uncalibrated) ---
    {36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
     52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // --- id 4: Echo (sequential default, uncalibrated) ---
    {36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
     52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // --- id 5: Stop-board (no key mapping) ---
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

// ---- Per-chip state ----
static uint8_t  chipAddr[MAX_CHIPS];   // I2C addresses of discovered chips
static int      numChips = 0;
static uint16_t prevState[MAX_CHIPS];  // Last known GPIO state (1 = high/released)

// ---- Low-level I2C helpers ----

static bool mcp_write(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

// Read GPIOA and GPIOB in a single sequential transaction.
// Returns a 16-bit word: bits [7:0] = port A, bits [15:8] = port B.
// Reading GPIO also clears the chip's interrupt flag.
static uint16_t mcp_read_gpio(uint8_t addr) {
    Wire.beginTransmission(addr);
    Wire.write(REG_GPIOA);
    Wire.endTransmission(false);          // Repeated-start (no STOP)
    Wire.requestFrom(addr, (uint8_t)2);
    uint8_t a = Wire.available() ? Wire.read() : 0;
    uint8_t b = Wire.available() ? Wire.read() : 0;
    return (uint16_t)a | ((uint16_t)b << 8);
}

// ---- Public API ----

extern "C" {

void key_scanner_begin() {
    // Load hardware identity from NVS (defaults to 0 = Pedal if never set)
    {
        Preferences prefs;
        prefs.begin("organ", true);
        hardwareId = prefs.getInt("hw_id", 0);
        prefs.end();
    }
    Log.printf("KeyScan: hardware_id=%d  CAN channel=%d\n", hardwareId, hardwareId);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(400000);  // 400 kHz I2C fast-mode

    // Shared INT line is active-low; external 4.7 kΩ pull-up is already fitted.
    pinMode(PIN_I2C_INT, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_I2C_INT), onInterrupt, FALLING);

    // Probe addresses 0x20–0x27 and configure each chip found.
    numChips = 0;
    for (int i = 0; i < MAX_CHIPS; i++) {
        uint8_t addr = MCP23017_BASE_ADDR + i;
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() != 0) continue;   // Nothing at this address

        // All pins → inputs
        mcp_write(addr, REG_IODIRA,   0xFF);
        mcp_write(addr, REG_IODIRB,   0xFF);

        // Enable internal pull-ups on all pins
        mcp_write(addr, REG_GPPUA,    0xFF);
        mcp_write(addr, REG_GPPUB,    0xFF);

        // Interrupt on any change from previous state (captures both press & release)
        mcp_write(addr, REG_INTCONA,  0x00);
        mcp_write(addr, REG_INTCONB,  0x00);

        // IOCON: MIRROR + open-drain (safe to wire INT pins together)
        mcp_write(addr, REG_IOCONA,   IOCON_VALUE);

        // Enable interrupt on all pins
        mcp_write(addr, REG_GPINTENA, 0xFF);
        mcp_write(addr, REG_GPINTENB, 0xFF);

        // Read initial state (also clears any pending interrupt on this chip)
        chipAddr[numChips]  = addr;
        prevState[numChips] = mcp_read_gpio(addr);

        Log.printf("KeyScan: chip %d at 0x%02X  initial=0x%04X\n",
                   numChips, addr, prevState[numChips]);
        numChips++;
    }
    Log.printf("KeyScan: %d MCP23017 chip(s) found\n", numChips);
    if (numChips != expectedChips[hardwareId]) {
        Log.printf("KeyScan: WARNING – expected %d chip(s) for hw_id=%d, found %d\n",
                   expectedChips[hardwareId], hardwareId, numChips);
    }

    // Force an immediate scan so our stored state is definitely in sync.
    intFlag = true;
}

void key_scanner_update() {
    // Fast path: INT is still deasserted (high) and no ISR flag → nothing to do.
    if (!intFlag && digitalRead(PIN_I2C_INT) == HIGH) return;
    intFlag = false;

    for (int c = 0; c < numChips; c++) {
        uint16_t cur     = mcp_read_gpio(chipAddr[c]);   // clears this chip's INT
        uint16_t changed = cur ^ prevState[c];
        if (!changed) continue;

        for (int pin = 0; pin < KEYS_PER_CHIP; pin++) {
            if (!(changed & (1u << pin))) continue;

            // Map physical wiring address to the correct MIDI note number.
            uint8_t raw     = (uint8_t)c * (uint8_t)KEYS_PER_CHIP + (uint8_t)pin;
            uint8_t note    = keymaps[hardwareId][raw];
            bool    pressed = !(cur & (1u << pin));  // active-low: 0 = pressed

            if (pressed) {
                Log.printf("KeyScan: ON  chip=%d pin=%d note=%d\n", c, pin, note);
                can_send_note_on((uint8_t)hardwareId, note, 127);
            } else {
                Log.printf("KeyScan: OFF chip=%d pin=%d note=%d\n", c, pin, note);
                can_send_note_off((uint8_t)hardwareId, note, 64);
            }
        }
        prevState[c] = cur;
    }
}

int key_scanner_chip_count() {
    return numChips;
}

int key_scanner_get_hardware_id() {
    return hardwareId;
}

void key_scanner_set_hardware_id(int id) {
    if (id < 0 || id > 5) return;
    hardwareId = id;
    Preferences prefs;
    prefs.begin("organ", false);
    prefs.putInt("hw_id", id);
    prefs.end();
}

uint8_t key_scanner_get_can_channel() {
    return (uint8_t)hardwareId;
}

} // extern "C"
