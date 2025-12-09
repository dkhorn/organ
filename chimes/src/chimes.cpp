// chimes.cpp
#include <Arduino.h>
#include "driver/mcpwm.h"
#include "driver/sigmadelta.h"
#include "logger.h"

// ---------- User-tweakable envelope ----------
static const uint16_t KICK_DUTY = 100;      // % duty during lift
static const uint32_t KICK_MS   = 35;       // ms
static const uint32_t PWM_FREQ  = 4000;     // Hz (>=2 kHz to avoid whine)
static const uint8_t  LEDC_RES_BITS = 12;   // 12-bit resolution

// ---------- Channel mapping (0..20) ----------
// GPIO Pin Usage:
//   Chimes (21 channels): 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,35,36,37
//   MIDI Input: 38
//   Reserved for future SPI (damper control):
//     GPIO 39 - SCK (SPI Clock)
//     GPIO 40 - MOSI (SPI Data Out)
//     GPIO 41 - CS (SPI Chip Select)
//     GPIO 42 - Optional 2nd CS or MISO
//   Still available: 0,19,20,21,43,44,45,46,47,48 (use with caution on strapping pins)

enum class ChType : uint8_t { MCP, LEDC, SIGMA };

struct Chan {
  ChType type;
  // MCP
  mcpwm_unit_t unit;
  mcpwm_timer_t timer;
  mcpwm_operator_t opr; // MCPWM_OPR_A or _B
  // LEDC
  int ledcChan;
  int ledcPin;
  // SIGMA
  int sigmaChan;
  int sigmaPin;
};

static Chan CH[21] = {
  // 12 x MCPWM  (UNIT0 & UNIT1)
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A}, // 0 -> GPIO4
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B}, // 1 -> GPIO5
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A}, // 2 -> GPIO6
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_B}, // 3 -> GPIO7
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_2, MCPWM_OPR_A}, // 4 -> GPIO15
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_2, MCPWM_OPR_B}, // 5 -> GPIO16
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A}, // 6 -> GPIO17
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_B}, // 7 -> GPIO18
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A}, // 8 -> GPIO8
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_B}, // 9 -> GPIO3
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_2, MCPWM_OPR_A}, // 10 -> GPIO9
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_2, MCPWM_OPR_B}, // 11 -> GPIO10

  // 9 x LEDC (pins: 11,12,13,14,1,2,37,36,35)
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  0, 11}, // 12 -> 11
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  1, 12}, // 13 -> 12
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  2, 13}, // 14 -> 13
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  3, 14}, // 15 -> 14
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  4,  1}, // 16 -> 1
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  5,  2}, // 17 -> 2
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  6, 37}, // 18 -> 37
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  7, 36}, // 19 -> 36 (swapped from 35)
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  8, 35}, // 20 -> 35 (swapped from 36)
};

// GPIO map for MCP channels in order of the table above
static const int MCP_GPIO[12] = {4,5,6,7,15,16,17,18,8,3,9,10};

// ---------- Note-to-Channel Mapping ----------
// Maps note numbers (0-20) to physical channel numbers (0-20)
// This allows logical musical order independent of physical wiring
// Updated based on actual chime tuning test (after rewiring)
// Note: Only 20 physical chimes exist (0-19), channel 19 not connected
static const int NOTE_TO_CHANNEL[21] = {
  20, // note 0   -> channel 20 (GPIO 35) - placeholder, GPIO 35 doesn't work and there's no chime here
  13, // note 1 A  -> channel 13 (GPIO 12)
  12, // note 2 A# -> channel 12 (GPIO 11)
  11, // note 3 B  -> channel 11 (GPIO 10)
  10, // note 4 C  -> channel 10 (GPIO 9)
  9,  // note 5 C# -> channel 9  (GPIO 3)
  8,  // note 6 D  -> channel 8  (GPIO 8)
  7,  // note 7 D# -> channel 7  (GPIO 18)
  18, // note 8 E  -> channel 18 (GPIO 37)
  19, // note 9 F  -> channel 19 (GPIO 36)
  16, // note 10 F# -> channel 16 (GPIO 1)
  17, // note 11 G  -> channel 17 (GPIO 2)
  15, // note 12 G# -> channel 15 (GPIO 14)
  14, // note 13 A  -> channel 14 (GPIO 13)
  6,  // note 14 A# -> channel 6  (GPIO 17)
  5,  // note 15 B  -> channel 5  (GPIO 16)
  2,  // note 16 C  -> channel 2  (GPIO 6)
  3,  // note 17 C# -> channel 3  (GPIO 7)
  4,  // note 18 D  -> channel 4  (GPIO 15)
  1,  // note 19 D# -> channel 1  (GPIO 5)
  0   // note 20 E  -> channel 0  (GPIO 4)
};

// ---------- Calibration Data ----------
// Per-channel calibration: min and max duty percentage for velocity scaling
struct ChimeCalibration {
  uint8_t min_duty_pct;  // Minimum duty % (for velocity=1)
  uint8_t max_duty_pct;  // Maximum duty % (for velocity=127)
  uint8_t kick_ms_min;   // Kick hold time at max duty (shorter)
  uint8_t kick_ms_max;   // Kick hold time at min duty (longer)
};

static ChimeCalibration CALIBRATION[21] = {
  {50, 100, 35, 110}, // channel 0
  {50, 100, 35, 110}, // channel 1
  {54, 100, 35, 105}, // channel 2
  {58, 100, 35, 112}, // channel 3
  {51, 100, 35, 139}, // channel 4
  {56, 100, 35, 95}, // channel 5
  {58, 100, 35, 110}, // channel 6
  {56, 100, 35, 135}, // channel 7
  {53, 100, 35, 135}, // channel 8
  {60, 100, 35, 94}, // channel 9
  {59, 100, 35, 131}, // channel 10
  {50, 100, 35, 135}, // channel 11
  {60, 100, 35, 89}, // channel 12
  {65, 100, 35, 80}, // channel 13
  {58, 100, 35, 84}, // channel 14
  {63, 100, 35, 72}, // channel 15
  {53, 100, 35, 120}, // channel 16
  {51, 100, 35, 102}, // channel 17
  {66, 100, 35, 100}, // channel 18
  {46, 100, 35, 133}, // channel 19
  {46, 100, 35, 133}  // channel 20
};

// ---------- Simple strike state machine ----------
enum class StrikeState : uint8_t { IDLE, KICK };
struct Strike {
  StrikeState st = StrikeState::IDLE;
  uint32_t t0 = 0; // ms start time
  uint8_t duty_pct = 100; // Initial duty for this strike
  uint8_t kick_hold_ms = KICK_MS;  // Kick hold time in ms
};
static Strike S[21];

// ---------- Power Budget System ----------
// At 13V and 8 ohms, each chime draws 1.625A when active
// Maximum current budget: 10A -> ~6 chimes max (6 * 1.625 = 9.75A)
static const int MAX_CONCURRENT_CHIMES = 6;

static inline void setDutyPct(int ch, uint16_t dutyPct) {
  if (ch < 0 || ch >= 21) return;
  Chan &C = CH[ch];
  dutyPct = (dutyPct > 100) ? 100 : dutyPct;

  switch (C.type) {
    case ChType::MCP:
      if (C.opr == MCPWM_OPR_A)
        mcpwm_set_duty(C.unit, C.timer, MCPWM_OPR_A, dutyPct);
      else
        mcpwm_set_duty(C.unit, C.timer, MCPWM_OPR_B, dutyPct);
      break;

    case ChType::LEDC: {
      uint32_t maxv = (1u << LEDC_RES_BITS) - 1u;
      uint32_t val = (uint32_t)((dutyPct * maxv) / 100u);
      ledcWrite(C.ledcChan, val);
      break;
    }

    case ChType::SIGMA: {
      // Sigma-delta duty range: -128..+127 (roughly), map 0..100% to 0..120
      int d = (int)((dutyPct * 120) / 100u);
      if (d > 120) d = 120;
      sigmadelta_set_duty((sigmadelta_channel_t)C.sigmaChan, d);
      break;
    }
  }
}

static void initMCPWM() {
  // Route GPIOs
  // UNIT0: timers 0..2 -> GPIO12/13,14/15,16/17
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, MCP_GPIO[0]);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, MCP_GPIO[1]);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, MCP_GPIO[2]);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1B, MCP_GPIO[3]);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM2A, MCP_GPIO[4]);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM2B, MCP_GPIO[5]);

  // UNIT1: timers 0..2 -> GPIO4/5,6/7,8/9
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, MCP_GPIO[6]);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0B, MCP_GPIO[7]);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1A, MCP_GPIO[8]);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1B, MCP_GPIO[9]);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM2A, MCP_GPIO[10]);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM2B, MCP_GPIO[11]);

  mcpwm_config_t cfg{};
  cfg.frequency = PWM_FREQ;
  cfg.cmpr_a = 0; // start off
  cfg.cmpr_b = 0;
  cfg.counter_mode = MCPWM_UP_COUNTER;
  cfg.duty_mode = MCPWM_DUTY_MODE_0;

  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &cfg);
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &cfg);
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_2, &cfg);

  mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &cfg);
  mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_1, &cfg);
  mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_2, &cfg);
}

static void initLEDC() {
  // Initialize all 9 LEDC channels (12-20)
  for (int i = 12; i <= 20; ++i) {
    pinMode(CH[i].ledcPin, OUTPUT);  // Explicitly set pin mode
    ledcSetup(i - 12, PWM_FREQ, LEDC_RES_BITS);       // use unique channels 0..8
    ledcAttachPin(CH[i].ledcPin, i - 12);
    CH[i].ledcChan = i - 12;
    ledcWrite(CH[i].ledcChan, 0);
  }
}


static void initSIGMA() {
//   sigmadelta_config_t sd{};
//   sd.channel    = (sigmadelta_channel_t)CH[20].sigmaChan; // CH20
//   sd.duty       = 0;             // start off
//   sd.prescale   = 0;             // default (high carrier); coil inductance will smooth it
//   sd.timer_sel  = SIGMADELTA_TIMER_0;
//   sigmadelta_config(&sd);
//   sigmadelta_set_pin((sigmadelta_channel_t)CH[20].sigmaChan, CH[20].sigmaPin);
}


// ---------- Public API ----------
extern "C" {

void chimes_begin() {
  // Zero states
  for (auto &s : S) { s.st = StrikeState::IDLE; s.t0 = 0; }

  initMCPWM();
  initLEDC();
  initSIGMA();

  // Ensure all outputs are 0% to start
  for (int i = 0; i < 21; ++i) setDutyPct(i, 0);
}

// Find oldest active chime and kill it
static void enforce_power_budget(int max_allowed) {
  int oldest_ch = -1;
  uint32_t oldest_t0 = UINT32_MAX;
  int active_count = 0;
  
  do {
    active_count = 0;
    oldest_ch = -1;
    oldest_t0 = UINT32_MAX;

    for (int ch = 0; ch < 21; ++ch) {
      if (S[ch].st == StrikeState::KICK) {
        active_count++;
        if (S[ch].t0 < oldest_t0) {
          oldest_t0 = S[ch].t0;
          oldest_ch = ch;
        }
      }
    }
    if (active_count > max_allowed && oldest_ch >= 0) {
      Log.printf("Power budget: Killing oldest chime %d (active=%d/%d)\n", 
                oldest_ch, active_count, MAX_CONCURRENT_CHIMES);
      
      // Force it to IDLE
      S[oldest_ch].st = StrikeState::IDLE;
      setDutyPct(oldest_ch, 0);
      active_count--;
    }
  } while (active_count > max_allowed);
}

void ring_chime_raw(int ch, int dutyPct, int kickHoldTimeMs) {
  if (ch < 0 || ch >= 21) return;
  
  // Check power budget - if at limit and this is a new strike, kill oldest chime
  if (S[ch].st == StrikeState::IDLE) {
    // Make room for a new strike by enforcing we have one less than max
    enforce_power_budget(MAX_CONCURRENT_CHIMES - 1);
  }
  
  // Start a new strike if idle (or restart current one)
  S[ch].st = StrikeState::KICK;
  S[ch].t0 = millis();
  S[ch].duty_pct = dutyPct;
  S[ch].kick_hold_ms = kickHoldTimeMs;

  Log.printf("%d ring_chime_raw: ch=%d duty=%d hold=%d\n", 
             S[ch].t0, ch, dutyPct, kickHoldTimeMs);
  setDutyPct(ch, dutyPct);
}

void ring_chime_by_channel(int ch, int velocity) {
  if (ch < 0 || ch >= 21) return;
  
  // Clamp velocity to valid MIDI range
  if (velocity < 1) velocity = 1;
  if (velocity > 127) velocity = 127;

  // Scale duty using calibration data
  // velocity=1 -> min_duty_pct, velocity=127 -> max_duty_pct
  const ChimeCalibration &cal = CALIBRATION[ch];
  uint16_t scaled_duty = cal.min_duty_pct + 
                        ((cal.max_duty_pct - cal.min_duty_pct) * (velocity - 1)) / 126;
  
  // Calculate kick time inversely proportional to duty
  // Higher duty (stronger) = shorter kick time
  // Lower duty (weaker) = longer kick time
  // Use reciprocal interpolation: 1/duty varies linearly with kick_ms
  // At max_duty -> kick_ms_min, at min_duty -> kick_ms_max
  
  // Calculate reciprocals (scaled by 10000 to maintain precision)
  float inv_duty = 10000.0f / scaled_duty;
  float inv_max_duty = 10000.0f / cal.max_duty_pct;
  float inv_min_duty = 10000.0f / cal.min_duty_pct;
  
  // Linear interpolation in reciprocal space
  float kick_ms_f = cal.kick_ms_min + 
                    (cal.kick_ms_max - cal.kick_ms_min) * 
                    (inv_duty - inv_max_duty) / (inv_min_duty - inv_max_duty);
  
  uint16_t kick_ms = (uint16_t)(kick_ms_f + 0.5f); // Round to nearest integer
  
  ring_chime_raw(ch, scaled_duty, kick_ms);
}

void ring_chime(int note, int velocity) {
  if (note < 0 || note >= 21) return;
  
  // Map note number to physical channel
  int ch = NOTE_TO_CHANNEL[note];
  ring_chime_by_channel(ch, velocity);
}

void chimes_all_off() {
  // Reset all chime plungers to idle state
  for (int ch = 0; ch < 21; ++ch) {
    S[ch].st = StrikeState::IDLE;
    S[ch].t0 = 0;
    setDutyPct(ch, 0);
  }
}

void chimes_loop() {
  const uint32_t now = millis();
  for (int ch = 0; ch < 21; ++ch) {
    Strike &st = S[ch];
    switch (st.st) {
      case StrikeState::IDLE:
        break;

      case StrikeState::KICK: {
        if (now - st.t0 >= st.kick_hold_ms) {
          // release (off)
          setDutyPct(ch, 0);
          st.st = StrikeState::IDLE;
          Log.printf("%d Chime %d: IDLE\n", now, ch);
        }
      } break;
    }
  }
}

} // extern "C"
