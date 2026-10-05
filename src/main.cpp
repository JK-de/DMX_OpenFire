#include "dmx_rx.h"
#include "fire.h"
#include "pins.h"
#include "pwm_out.h"
#include "status_led.h"
#include "ws2812.h"

#include <Arduino.h>
#include <hardware/clocks.h>

namespace {

uint32_t g_last_us = 0;
uint64_t g_acc_us = 0;

uint8_t read_dip() {
  uint8_t dip = 0;
  for (uint8_t bit = 0; bit < 4; ++bit) {
    const uint8_t pin = static_cast<uint8_t>(kDipMsbPin + bit);
    if (digitalRead(pin) == LOW) {
      dip = static_cast<uint8_t>(dip | (1u << (3u - bit)));
    }
  }
  return dip;
}

float seconds_since_boot() {
  const uint32_t now = micros();
  g_acc_us += static_cast<uint32_t>(now - g_last_us);
  g_last_us = now;
  return static_cast<float>(g_acc_us) * 1.0e-6f;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  pwm_out_begin();

  pinMode(kModePin, INPUT_PULLUP);
  for (uint8_t bit = 0; bit < 4; ++bit) {
    pinMode(static_cast<uint8_t>(kDipMsbPin + bit), INPUT_PULLUP);
  }
  analogReadResolution(12);

  dmx_rx_begin();
  status_led_begin();
  ws2812_begin();

  g_last_us = micros();
  const uint32_t hz = clock_get_hz(clk_sys);
  const uint32_t counts = static_cast<uint32_t>(pwm_out_top()) + 1u;
  const uint32_t pwm_hz = counts == 0 ? 0 : hz / counts;
  Serial.printf("clk %lu Hz  wrap %u  pwm %lu Hz\n", static_cast<unsigned long>(hz), pwm_out_top(),
                static_cast<unsigned long>(pwm_hz));
}

void loop() {
  const float time_s = seconds_since_boot();
  const uint8_t mode = digitalRead(kModePin) == LOW ? 0 : 1;
  const uint16_t start = static_cast<uint16_t>(1u + static_cast<uint16_t>(read_dip()) * 16u);

  uint8_t dmxs[16];
  dmx_rx_copy(start, dmxs);
  if (dmx_rx_take_frame()) {
    status_led_mark_packet();
  }

  uint16_t pots[3];
  for (uint8_t i = 0; i < 3; ++i) {
    pots[i] = static_cast<uint16_t>(analogRead(static_cast<uint8_t>(kPotPin0 + i)));
  }

  float levels[16];
  for (uint8_t i = 0; i < kPwmCount; ++i) {
    float level = fire(time_s, mode, i, dmxs, pots);
    if (level < 0.0f) {
      level = 0.0f;
    }
    if (level > 1.0f) {
      level = 1.0f;
    }
    levels[i] = level;
    pwm_out_write(i, level);
  }
  ws2812_show(levels);
  status_led_update();
}
