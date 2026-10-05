#include "noise.h"

#include <Arduino.h>

#include <math.h>

namespace {

const uint16_t __in_flash("perm") kPerm[65536] = {
#include "perm_table.inc"
};

float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

uint16_t perm_at(int32_t i) { return kPerm[static_cast<uint16_t>(static_cast<uint32_t>(i))]; }

float grad(uint16_t h, float x) { return (h & 1u) != 0 ? -x : x; }

}  // namespace

uint16_t perm(int16_t x) { return kPerm[static_cast<uint16_t>(x)]; }

float perlin_noise(float x) {
  const float x_floor = floorf(x);
  const int32_t i0 = static_cast<int32_t>(x_floor);
  const float t = x - x_floor;
  const float u = fade(t);
  const float g0 = grad(perm_at(i0), t);
  const float g1 = grad(perm_at(i0 + 1), t - 1.0f);
  return g0 + u * (g1 - g0);
}
