#pragma once

#include <stdint.h>

// Flash permutation of every value 0..65535. Index wraps as uint16.
uint16_t perm(int16_t x);

// 1D improved Perlin. About -1..1. Negative x uses floor.
float perlin_noise(float x);
