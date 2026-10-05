#include "fire.h"

#include "noise.h"

#include <math.h>

namespace {

struct FireIn {
  float coord;
  float brightness;
  float depth;
};

float clampf(float v, float lo, float hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

float to01(float n) { return clampf(n * 0.5f + 0.5f, 0.0f, 1.0f); }

FireIn inputs(float time_s, uint8_t mode, const uint8_t dmxs[16], const uint16_t pots[3]) {
  FireIn in;
  if (mode == 0) {
    const float pot_speed = pots[1] / 4095.0f;
    const float speed = 0.3f + pot_speed * (4.0f - 0.3f);
    in.coord = time_s * speed;
    in.brightness = pots[0] / 4095.0f;
    in.depth = pots[2] / 4095.0f;
  } else {
    in.coord = static_cast<float>(dmxs[0]) * 0.05f;
    in.brightness = static_cast<float>(dmxs[1]) / 255.0f;
    in.depth = static_cast<float>(dmxs[2]) / 255.0f;
  }
  in.brightness = clampf(in.brightness, 0.0f, 1.0f);
  in.depth = clampf(in.depth, 0.0f, 1.0f);
  return in;
}

__attribute__((used)) float fire_fbm(float coord, float brightness, float depth, uint8_t idx) {
  const float x = static_cast<float>(idx);
  const float n0 = to01(perlin_noise(coord + x * 4.7f));
  const float n1 = to01(perlin_noise(coord * 2.3f + x * 9.1f + 20.0f));
  const float n2 = to01(perlin_noise(coord * 5.1f + x * 2.3f + 50.0f));
  float flame = n0 * 0.55f + n1 * 0.30f + n2 * 0.15f;
  flame = powf(clampf(flame, 0.0f, 1.0f), 0.6f + depth);
  return brightness * flame;
}

__attribute__((used)) float fire_candle(float coord, float brightness, float depth, uint8_t idx) {
  const float n = to01(perlin_noise(coord + static_cast<float>(idx) * 4.7f));
  const float flame = powf(n, 0.35f + depth);
  return brightness * flame;
}

uint16_t value_hash(int32_t frame, uint8_t idx) {
  const uint32_t mixed = static_cast<uint32_t>(frame) + static_cast<uint32_t>(idx) * 4099u;
  return perm(static_cast<int16_t>(static_cast<uint16_t>(mixed)));
}

__attribute__((used)) float fire_value(float coord, float brightness, float depth, uint8_t idx) {
  const float base = floorf(coord);
  const int32_t i0 = static_cast<int32_t>(base);
  float frac = coord - base;
  frac = frac * frac * (3.0f - 2.0f * frac);
  const float a = static_cast<float>(value_hash(i0, idx)) / 65535.0f;
  const float b = static_cast<float>(value_hash(i0 + 1, idx)) / 65535.0f;
  float flame = a + (b - a) * frac;
  flame = 1.0f - depth * (1.0f - flame);
  return brightness * clampf(flame, 0.0f, 1.0f);
}

__attribute__((used)) float fire_column(float coord, float brightness, float depth, uint8_t idx) {
  const float x = static_cast<float>(idx);
  const float n = to01(perlin_noise(coord - x * 0.45f));
  const float falloff = 1.0f - x / 16.0f;
  float flame = n * falloff;
  flame = powf(clampf(flame, 0.0f, 1.0f), 0.6f + depth);
  return brightness * flame;
}

__attribute__((used)) float fire_sine(float coord, float brightness, float depth, uint8_t idx) {
  const float x = static_cast<float>(idx);
  const float a = sinf(coord * 6.2831853f + x * 0.55f);
  const float b = sinf(coord * 14.4513f + x * 1.7f);
  float flame = 0.5f + (0.35f * a + 0.15f * b) * (0.25f + 0.75f * depth);
  return brightness * clampf(flame, 0.0f, 1.0f);
}

using FireFn = float (*)(float, float, float, uint8_t);

const FireFn kAlgos[] = {nullptr, fire_fbm, fire_candle, fire_value, fire_column, fire_sine};

}  // namespace

float fire(float time_s, uint8_t mode, uint8_t idx, const uint8_t dmxs[16], const uint16_t pots[3]) {
  const FireIn in = inputs(time_s, mode, dmxs, pots);
  const int algo = FIRE_ALGO;
  if (algo >= FIRE_FBM && algo <= FIRE_SINE) {
    return kAlgos[algo](in.coord, in.brightness, in.depth, idx);
  }
  return fire_fbm(in.coord, in.brightness, in.depth, idx);
}
