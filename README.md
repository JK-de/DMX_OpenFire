# DMX_OpenFire

Raspberry Pi Pico firmware that emulates fire on 16 high-active PWM outputs and a WS2812 stripe. A DIP switch sets the DMX-512 start address. A mode switch selects manual pots or DMX. The onboard LED shows a heartbeat and incoming DMX.

USB serial speaks the ENTTEC DMX USB Pro widget protocol and does not speak Dragonframe DMC. Open the COM port as an ENTTEC DMX USB Pro at 57600 baud. A label-6 universe is used for one second in place of the XLR input; after that the wire values are kept. The USB id stays a Raspberry Pi Pico. The name strings are manufacturer ENTTEC and product DMX USB PRO.

QLC+ and OLA open Pro widgets through libftdi, so they will not send to this CDC port. Dragonframe may only list FTDI COM ports. There is no text on this serial port.

The GitHub remote is https://github.com/JK-de/DMX_OpenFire.

Hardware drawing, RS-485 receiver, bulb drivers, and the loop rate: [docs/hardware.md](docs/hardware.md).

## Pinout

| Function | GPIO | Notes |
|---|---|---|
| PWM 0..15 | 0..15 | High = on, about 18 kHz |
| Mode | 16 | Pull-up. Low = manual, high = DMX |
| DMX-512 in | 17 | UART0 RX only, 250000 8N2 |
| DIP | 18..21 | Pull-up, closed = 1. GPIO 21 is LSB |
| WS2812 | 22 | PIO, RGB wire order, 64 pixels |
| Onboard LED | 25 | Heartbeat, flash on each DMX frame |
| Pots | 26..28 | ADC, 0..4095 |

## DMX address

Start channel = `1 + dip * 16`. DIP 0 reads channels 1..16. DIP 15 reads channels 241..256. The address is live.

In DMX mode, `fire` uses the first three channels of that window:

- channel start + 0: animation frame
- channel start + 1: brightness
- channel start + 2: depth

The same frame value always produces the same levels. There is no heat memory.

In manual mode the clock is seconds since power-up. Pot 0 is brightness, pot 1 is speed (about 0.3..4 noise units per second), pot 2 is depth.

## PWM

System clock is 133 MHz, PWM divider is 1. The wrap is `clock / 18000 - 1`, which is 7387 counts of wrap and 7388 steps at 18002 Hz when the clock is 133 MHz. Level 0 is fully off. Level 1 is fully on.

## WS2812

The 16 PWM levels are mapped through a piecewise RGB ramp, then those 16 colors are sent four times (64 pixels) in one transfer:

- 0.00 black `(0, 0, 0)`
- 0.25 dark red `(80, 0, 0)`
- 0.50 red-orange `(200, 40, 0)`
- 0.75 orange `(255, 96, 0)`
- 1.00 yellow `(255, 180, 0)`

Wire order is R, then G, then B.

## How often each LED is updated

`loop` calls `fire` once per PWM channel, then sends the stripe. The look itself is not chosen yet. The call rate does not depend on that choice, as long as `fire` stays cheap next to the LED transfer.

The stripe is 16 colors repeated 4 times, so 64 pixels. Each pixel is 24 bits at 800 kHz:

```
64 * 24 / 800000 = 1.92 ms
```

That transfer is blocking, and it happens every pass. DIP reads, three ADC samples, and sixteen `fire` calls add roughly 0.3 ms. One pass is therefore about 2.2 ms, which is about **450 `fire` calls per LED per second**. If the stripe were the only work, the ceiling would be about 520 calls per second (`1 / 1.92 ms`).

A flame only needs on the order of 25 to 50 visible changes per second. The loop recalculates each LED about ten times faster than that.

## Build

Board: Raspberry Pi Pico. Core: earlephilhower Arduino Pico via PlatformIO (`env:rpipico`).

```bash
python -m platformio run -e rpipico
python -m platformio run -e rpipico --target upload
```

Regenerate the permutation table with `python tools/gen_perm.py`. The checked-in table already matches that script.

## License

Copyright (c) 2026 Jochen Krapf \<jk@nerd2nerd.org\>

Licensed under the [MIT License](LICENSE).
