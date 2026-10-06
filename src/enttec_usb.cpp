#include "enttec_usb.h"

#include "dmx_rx.h"

#include <Arduino.h>
#include <USB.h>

#include <cstring>

namespace {

constexpr uint8_t kStart = 0x7E;
constexpr uint8_t kEnd = 0xE7;
constexpr uint8_t kLabelGetParams = 3;
constexpr uint8_t kLabelSetParams = 4;
constexpr uint8_t kLabelSendDmx = 6;
constexpr uint8_t kLabelGetSerial = 10;
constexpr uint16_t kMaxLen = 600;

enum class State : uint8_t { Sync, Label, LenL, LenH, Data, End };

State g_state = State::Sync;
uint8_t g_label = 0;
uint16_t g_len = 0;
uint16_t g_got = 0;
uint8_t g_data[kMaxLen];

// Firmware 1.44, break 9 * 10.67 us, mark-after-break 1, 40 packets/s.
uint8_t g_params[5] = {0x44, 0x01, 9, 1, 40};
constexpr uint8_t kSerial[4] = {0x01, 0x00, 0x00, 0x00};

void reply(uint8_t label, const uint8_t* data, uint16_t len) {
  const uint8_t hdr[4] = {kStart, label, static_cast<uint8_t>(len), static_cast<uint8_t>(len >> 8)};
  Serial.write(hdr, sizeof(hdr));
  if (len > 0 && data != nullptr) {
    Serial.write(data, len);
  }
  Serial.write(kEnd);
}

void handle() {
  switch (g_label) {
    case kLabelGetParams:
      reply(kLabelGetParams, g_params, sizeof(g_params));
      break;
    case kLabelSetParams:
      if (g_len >= sizeof(g_params)) {
        memcpy(g_params, g_data, sizeof(g_params));
      }
      break;
    case kLabelSendDmx:
      if (g_len >= 1 && g_data[0] == 0) {
        dmx_rx_publish_usb(g_data + 1, static_cast<uint16_t>(g_len - 1));
      }
      break;
    case kLabelGetSerial:
      reply(kLabelGetSerial, kSerial, sizeof(kSerial));
      break;
    default:
      break;
  }
}

void feed(uint8_t byte) {
  switch (g_state) {
    case State::Sync:
      if (byte == kStart) {
        g_state = State::Label;
      }
      break;
    case State::Label:
      g_label = byte;
      g_state = State::LenL;
      break;
    case State::LenL:
      g_len = byte;
      g_state = State::LenH;
      break;
    case State::LenH:
      g_len = static_cast<uint16_t>(g_len | (static_cast<uint16_t>(byte) << 8));
      g_got = 0;
      if (g_len > kMaxLen) {
        g_state = State::Sync;
      } else if (g_len == 0) {
        g_state = State::End;
      } else {
        g_state = State::Data;
      }
      break;
    case State::Data:
      g_data[g_got++] = byte;
      if (g_got >= g_len) {
        g_state = State::End;
      }
      break;
    case State::End:
      if (byte == kEnd) {
        handle();
      }
      g_state = byte == kStart ? State::Label : State::Sync;
      break;
  }
}

}  // namespace

void enttec_usb_begin() {
  // Compile-time -DUSB_PRODUCT cannot carry a space without breaking the link line.
  // Re-enumerate so the host reads these strings instead of "Raspberry Pi" / "Pico".
  USB.setManufacturer("ENTTEC");
  USB.setProduct("DMX USB PRO");
  USB.disconnect();
  USB.connect();
  Serial.begin(57600);
}

void enttec_usb_poll() {
  while (Serial.available() > 0) {
    feed(static_cast<uint8_t>(Serial.read()));
  }
}
