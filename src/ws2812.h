#pragma once

void ws2812_begin();

// Map 16 levels in 0..1 through the fire LUT and send them four times.
void ws2812_show(const float levels[16]);
