#include "ws2812.h"

#include "pins.h"

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>

namespace {

constexpr uint32_t kBitHz = 800000;
constexpr uint32_t kCyclesPerBit = 10;
constexpr int kRepeat = 4;

// WS2812 800 kHz, 10 cycles/bit. Same program as SliderMC status_neopixel.
constexpr uint16_t kInstr[] = {
    0x6221,  // out x, 1     side 0 [2]
    0x1123,  // jmp !x, 3    side 1 [1]
    0x1400,  // jmp 0        side 1 [4]
    0xa442,  // nop          side 0 [4]
};

const pio_program_t kProg = {
    .instructions = kInstr,
    .length = 4,
    .origin = -1,
};

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

// 0, 1/4, 1/2, 3/4, 1. Black, dark red, red-orange, orange, yellow.
constexpr Rgb kStops[] = {
    {0, 0, 0},
    {80, 0, 0},
    {200, 40, 0},
    {255, 96, 0},
    {255, 180, 0},
};

PIO g_pio = pio0;
int g_sm = -1;
bool g_ok = false;

uint8_t lerp8(uint8_t a, uint8_t b, float t) {
  const float v = static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t;
  if (v <= 0.0f) {
    return 0;
  }
  if (v >= 255.0f) {
    return 255;
  }
  return static_cast<uint8_t>(v + 0.5f);
}

Rgb color_from_level(float level) {
  if (level < 0.0f) {
    level = 0.0f;
  }
  if (level > 1.0f) {
    level = 1.0f;
  }
  const float scaled = level * 4.0f;
  int seg = static_cast<int>(scaled);
  if (seg > 3) {
    seg = 3;
  }
  const float t = scaled - static_cast<float>(seg);
  const Rgb a = kStops[seg];
  const Rgb b = kStops[seg + 1];
  return Rgb{lerp8(a.r, b.r, t), lerp8(a.g, b.g, t), lerp8(a.b, b.b, t)};
}

void put_rgb(Rgb c) {
  const uint32_t packed = (static_cast<uint32_t>(c.r) << 16) | (static_cast<uint32_t>(c.g) << 8) |
                          static_cast<uint32_t>(c.b);
  pio_sm_put_blocking(g_pio, static_cast<uint>(g_sm), packed << 8);
}

}  // namespace

void ws2812_begin() {
  g_ok = false;
  g_sm = -1;
  g_pio = pio0;
  const int sm = static_cast<int>(pio_claim_unused_sm(g_pio, false));
  if (sm < 0 || !pio_can_add_program(g_pio, &kProg)) {
    if (sm >= 0) {
      pio_sm_unclaim(g_pio, static_cast<uint>(sm));
    }
    return;
  }
  const uint off = pio_add_program(g_pio, &kProg);
  const uint pin = kWs2812Pin;
  pio_gpio_init(g_pio, pin);
  pio_sm_set_consecutive_pindirs(g_pio, static_cast<uint>(sm), pin, 1, true);

  pio_sm_config cfg = pio_get_default_sm_config();
  sm_config_set_wrap(&cfg, off, off + 3u);
  sm_config_set_sideset(&cfg, 1, false, false);
  sm_config_set_sideset_pins(&cfg, pin);
  sm_config_set_out_shift(&cfg, false, true, 24);
  sm_config_set_fifo_join(&cfg, PIO_FIFO_JOIN_TX);
  const float div =
      static_cast<float>(clock_get_hz(clk_sys)) / (static_cast<float>(kBitHz) * static_cast<float>(kCyclesPerBit));
  sm_config_set_clkdiv(&cfg, div);
  pio_sm_init(g_pio, static_cast<uint>(sm), off, &cfg);
  pio_sm_set_enabled(g_pio, static_cast<uint>(sm), true);
  g_sm = sm;
  g_ok = true;
}

void ws2812_show(const float levels[16]) {
  if (!g_ok || levels == nullptr) {
    return;
  }
  Rgb pixels[16];
  for (int i = 0; i < 16; ++i) {
    pixels[i] = color_from_level(levels[i]);
  }
  for (int rep = 0; rep < kRepeat; ++rep) {
    for (int i = 0; i < 16; ++i) {
      put_rgb(pixels[i]);
    }
  }
}
