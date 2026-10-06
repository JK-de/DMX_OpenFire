#pragma once

#include <stdint.h>

void dmx_rx_begin();

// Channel bytes after a NULL start code. Slots past count are cleared.
void dmx_rx_publish_usb(const uint8_t* slots, uint16_t count);

// True once for each published DMX frame. Clears the flag.
bool dmx_rx_take_frame();

// Copy 16 slots starting at DMX channel start_channel_1 (1-based).
void dmx_rx_copy(uint16_t start_channel_1, uint8_t out[16]);
