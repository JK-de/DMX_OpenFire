#include "pwm_out.h"

#include "pins.h"

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

namespace {

uint16_t g_top = 1;

}  // namespace

void pwm_out_begin() {
  uint32_t cycles = clock_get_hz(clk_sys) / kPwmTargetHz;
  if (cycles < 2) {
    cycles = 2;
  }
  if (cycles > 65536u) {
    cycles = 65536u;
  }
  g_top = static_cast<uint16_t>(cycles - 1u);

  pwm_config cfg = pwm_get_default_config();
  pwm_config_set_clkdiv(&cfg, 1.0f);
  pwm_config_set_wrap(&cfg, g_top);

  bool slice_inited[8] = {};
  for (uint8_t i = 0; i < kPwmCount; ++i) {
    const uint pin = i;
    const uint slice = pwm_gpio_to_slice_num(pin);
    if (slice < 8 && !slice_inited[slice]) {
      pwm_init(slice, &cfg, true);
      slice_inited[slice] = true;
    }
    gpio_set_function(pin, GPIO_FUNC_PWM);
    gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_8MA);
    pwm_set_gpio_level(pin, 0);
  }
}

uint16_t pwm_out_top() { return g_top; }

void pwm_out_write(uint8_t idx, float level) {
  if (idx >= kPwmCount) {
    return;
  }
  if (level < 0.0f) {
    level = 0.0f;
  }
  if (level > 1.0f) {
    level = 1.0f;
  }
  const uint32_t counts = static_cast<uint32_t>(g_top) + 1u;
  uint32_t ticks = static_cast<uint32_t>(level * static_cast<float>(counts) + 0.5f);
  if (ticks > counts) {
    ticks = counts;
  }
  pwm_set_gpio_level(idx, static_cast<uint16_t>(ticks));
}
