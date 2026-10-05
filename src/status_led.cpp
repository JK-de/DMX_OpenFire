#include "status_led.h"

#include "pins.h"

#include <Arduino.h>

namespace {

uint32_t g_boot_until_ms = 0;
uint32_t g_packet_until_ms = 0;
uint32_t g_ready_anchor_ms = 0;

}  // namespace

void status_led_begin() {
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);
  const uint32_t now = millis();
  g_boot_until_ms = now + kLedBootOffMs;
  g_packet_until_ms = 0;
  g_ready_anchor_ms = now;
}

void status_led_mark_packet() {
  const uint32_t now = millis();
  if (now < g_boot_until_ms) {
    return;
  }
  g_packet_until_ms = now + kLedPacketPulseMs;
  g_ready_anchor_ms = now;
  digitalWrite(kLedPin, HIGH);
}

void status_led_update() {
  const uint32_t now = millis();
  if (now < g_boot_until_ms) {
    digitalWrite(kLedPin, LOW);
    return;
  }
  if (now < g_packet_until_ms) {
    digitalWrite(kLedPin, HIGH);
    return;
  }
  const uint32_t since = now - g_ready_anchor_ms;
  const bool on = (since % kLedReadyPeriodMs) < kLedReadyPulseMs;
  digitalWrite(kLedPin, on ? HIGH : LOW);
}
