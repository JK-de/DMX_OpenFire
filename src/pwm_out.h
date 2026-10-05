#pragma once

#include <stdint.h>

void pwm_out_begin();
uint16_t pwm_out_top();
void pwm_out_write(uint8_t idx, float level);
