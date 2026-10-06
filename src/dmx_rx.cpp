#include "dmx_rx.h"

#include "pins.h"

#include <Arduino.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/regs/uart.h>
#include <hardware/sync.h>
#include <hardware/uart.h>

#include <cstring>

namespace {

constexpr uint32_t kUsbHoldMs = 1000;

uint8_t g_frame[2][512];
uint8_t g_usb[512];
uint8_t g_building = 0;
volatile uint8_t g_stable = 0;
volatile bool g_frame_ready = false;
uint32_t g_usb_ms = 0;
bool g_usb_valid = false;
uint16_t g_slot = 0;
bool g_in_data = false;
bool g_expect_start = true;

void publish_frame() {
  if (!g_in_data) {
    return;
  }
  for (uint16_t i = g_slot; i < 512; ++i) {
    g_frame[g_building][i] = 0;
  }
  g_stable = g_building;
  g_building ^= 1u;
  g_slot = 0;
  g_in_data = false;
  g_frame_ready = true;
}

void on_uart0_rx() {
  while (uart_is_readable(uart0)) {
    const uint32_t dr = uart_get_hw(uart0)->dr;
    const uint8_t byte = static_cast<uint8_t>(dr & 0xFFu);
    const bool brk = (dr & UART_UARTDR_BE_BITS) != 0;
    if (brk) {
      publish_frame();
      g_slot = 0;
      g_in_data = false;
      g_expect_start = true;
      continue;
    }
    if (g_expect_start) {
      g_expect_start = false;
      g_slot = 0;
      g_in_data = (byte == 0);
      continue;
    }
    if (!g_in_data) {
      continue;
    }
    if (g_slot < 512) {
      g_frame[g_building][g_slot++] = byte;
    }
    if (g_slot >= 512) {
      publish_frame();
    }
  }
}

}  // namespace

void dmx_rx_begin() {
  uart_init(uart0, 250000);
  uart_set_format(uart0, 8, 2, UART_PARITY_NONE);
  uart_set_hw_flow(uart0, false, false);
  uart_set_fifo_enabled(uart0, true);
  gpio_set_function(kDmxRxPin, GPIO_FUNC_UART);

  irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx);
  irq_set_enabled(UART0_IRQ, true);
  uart_set_irq_enables(uart0, true, false);
}

void dmx_rx_publish_usb(const uint8_t* slots, uint16_t count) {
  if (count > 512) {
    count = 512;
  }
  if (slots != nullptr && count > 0) {
    memcpy(g_usb, slots, count);
  }
  if (count < 512) {
    memset(g_usb + count, 0, static_cast<size_t>(512 - count));
  }
  g_usb_ms = millis();
  if (g_usb_ms == 0) {
    g_usb_ms = 1;
  }
  g_usb_valid = true;
  g_frame_ready = true;
}

bool dmx_rx_take_frame() {
  const uint32_t save = save_and_disable_interrupts();
  const bool got = g_frame_ready;
  g_frame_ready = false;
  restore_interrupts(save);
  return got;
}

void dmx_rx_copy(uint16_t start_channel_1, uint8_t out[16]) {
  const uint16_t base = start_channel_1 < 1 ? 0 : static_cast<uint16_t>(start_channel_1 - 1);
  const bool usb = g_usb_valid && static_cast<uint32_t>(millis() - g_usb_ms) < kUsbHoldMs;
  if (usb) {
    for (uint8_t i = 0; i < 16; ++i) {
      const uint16_t ch = static_cast<uint16_t>(base + i);
      out[i] = ch < 512 ? g_usb[ch] : 0;
    }
    return;
  }
  const uint32_t save = save_and_disable_interrupts();
  const uint8_t* src = g_frame[g_stable];
  for (uint8_t i = 0; i < 16; ++i) {
    const uint16_t ch = static_cast<uint16_t>(base + i);
    out[i] = ch < 512 ? src[ch] : 0;
  }
  restore_interrupts(save);
}
