#pragma once

#include <stdint.h>

// Raspberry Pi Pico. GPIO 21 is the DIP LSB. Mode is low = manual.

constexpr uint8_t kPwmCount = 16;
constexpr uint8_t kModePin = 16;
constexpr uint8_t kDmxRxPin = 17;
constexpr uint8_t kDipMsbPin = 18;
constexpr uint8_t kWs2812Pin = 22;
constexpr uint8_t kLedPin = 25;
constexpr uint8_t kPotPin0 = 26;

constexpr uint32_t kPwmTargetHz = 18000;

constexpr uint32_t kLedBootOffMs = 500;
constexpr uint32_t kLedPacketPulseMs = 40;
constexpr uint32_t kLedReadyPeriodMs = 1000;
constexpr uint32_t kLedReadyPulseMs = 15;
