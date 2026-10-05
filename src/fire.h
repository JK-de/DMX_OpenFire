#pragma once

#include <stdint.h>

// Change FIRE_ALGO and rebuild to compare looks. Every algorithm is in fire.cpp.
#define FIRE_FBM 1
#define FIRE_CANDLE 2
#define FIRE_VALUE 3
#define FIRE_COLUMN 4
#define FIRE_SINE 5

#ifndef FIRE_ALGO
#define FIRE_ALGO FIRE_FBM
#endif

// Stateless. mode 0 uses time_s and pots. mode 1 uses dmxs[0..2] as frame, brightness, depth.
float fire(float time_s, uint8_t mode, uint8_t idx, const uint8_t dmxs[16], const uint16_t pots[3]);
