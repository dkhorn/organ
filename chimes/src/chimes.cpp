// chimes.cpp
#include <Arduino.h>
#include "driver/mcpwm.h"
#include "driver/sigmadelta.h"

// ---------- User-tweakable envelope ----------
static const uint16_t KICK_DUTY = 100;      // % duty during lift
static const uint16_t HOLD_DUTY = 20;       // % duty during short hold
static const uint32_t KICK_MS   = 1500;       // ms
static const uint32_t HOLD_MS   = 10;       // ms
static const uint32_t PWM_FREQ  = 4000;     // Hz (>=2 kHz to avoid whine)
static const uint8_t  LEDC_RES_BITS = 12;   // 12-bit resolution

// ---------- Channel mapping (0..20) ----------
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
  // 12 x MCPWM  (UNIT0: 12/13,14/15,16/17;  UNIT1: 4/5,6/7,8/9)
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A}, // 0 -> GPIO12
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B}, // 1 -> GPIO13
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A}, // 2 -> GPIO14
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_B}, // 3 -> GPIO15
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_2, MCPWM_OPR_A}, // 4 -> GPIO16
  {ChType::MCP, MCPWM_UNIT_0, MCPWM_TIMER_2, MCPWM_OPR_B}, // 5 -> GPIO17
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A}, // 6 -> GPIO4
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_B}, // 7 -> GPIO5
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A}, // 8 -> GPIO6
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_B}, // 9 -> GPIO7
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_2, MCPWM_OPR_A}, // 10 -> GPIO8
  {ChType::MCP, MCPWM_UNIT_1, MCPWM_TIMER_2, MCPWM_OPR_B}, // 11 -> GPIO9

  // 8 x LEDC  (pins: 1,2,3,10,11,18,35,36)
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  0,  1}, // 12
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  1,  2}, // 13
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  2,  3}, // 14
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  3, 10}, // 15
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  4, 11}, // 16
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  5, 18}, // 17
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  6, 35}, // 18
  {ChType::LEDC, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX,  7, 36}, // 19

  // 1 x Sigma-Delta (pin 38)
  // {ChType::SIGMA, MCPWM_UNIT_MAX, MCPWM_TIMER_MAX, MCPWM_OPR_MAX, -1, -1,  SIGMADELTA_CHANNEL_0, 38} // 20
};

// GPIO map for MCP channels in order of the table above
static const int MCP_GPIO[12] = {12,13,14,15,16,17,4,5,6,7,8,9};

// ---------- Simple strike state machine ----------
enum class StrikeState : uint8_t { IDLE, KICK, HOLD };
struct Strike {
  StrikeState st = StrikeState::IDLE;
  uint32_t t0 = 0; // ms start time
};
static Strike S[21];

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
  // One timer shared by all 8 channels
  ledcSetup(0, PWM_FREQ, LEDC_RES_BITS);
  ledcAttachPin(CH[12].ledcPin, 0);
  for (int i = 13; i <= 19; ++i) {
    ledcSetup(i - 12, PWM_FREQ, LEDC_RES_BITS);       // use unique channels 0..7
    ledcAttachPin(CH[i].ledcPin, i - 12);
    CH[i].ledcChan = i - 12;
    ledcWrite(CH[i].ledcChan, 0);
  }
  // ensure first one also zero
  ledcWrite(CH[12].ledcChan, 0);
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

void ring_chime(int ch) {
  if (ch < 0 || ch >= 21) return;
  // Start a new strike if idle (or restart current one)
  S[ch].st = StrikeState::KICK;
  S[ch].t0 = millis();
  setDutyPct(ch, KICK_DUTY);
}

void chimes_loop() {
  const uint32_t now = millis();
  for (int ch = 0; ch < 21; ++ch) {
    Strike &st = S[ch];
    switch (st.st) {
      case StrikeState::IDLE:
        break;

      case StrikeState::KICK: {
        if (now - st.t0 >= KICK_MS) {
          // move to HOLD/decay
          st.st = StrikeState::HOLD;
          st.t0 = now;
          setDutyPct(ch, HOLD_DUTY);
          // Serial.printf("Chime %d: HOLD\n", ch);
        }
      } break;

      case StrikeState::HOLD: {
        if (now - st.t0 >= HOLD_MS) {
          // release (off)
          setDutyPct(ch, 0);
          st.st = StrikeState::IDLE;
          // Serial.printf("Chime %d: IDLE\n", ch);
        }
      } break;
    }
  }
}

} // extern "C"
