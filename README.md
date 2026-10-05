# DMX_OpenFire

Raspberry Pi Pico firmware that emulates fire on 16 high-active PWM outputs and a WS2812 stripe. A DIP switch sets the DMX-512 start address. A mode switch selects manual pots or DMX. The onboard LED shows a heartbeat and incoming DMX.

There is no Dragonframe DMC motion protocol and no USB DMX widget in this build. USB CDC prints the measured clock, PWM wrap, and carrier once at boot.

The GitHub remote is not configured.

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

## Fire looks

`FIRE_ALGO` in [src/fire.h](src/fire.h) selects the look. The default is `FIRE_FBM`. Change it and rebuild.

| Value | Name | Look |
|---|---|---|
| 1 | `FIRE_FBM` | Three Perlin octaves, mostly bright |
| 2 | `FIRE_CANDLE` | One slow Perlin octave |
| 3 | `FIRE_VALUE` | Hashed steps from the permutation table |
| 4 | `FIRE_COLUMN` | Noise sheared by channel, darker at the top |
| 5 | `FIRE_SINE` | Two sines, for a wiring check |

`perm()` is a flash table of all values 0..65535, shuffled once with a fixed seed (`tools/gen_perm.py`). `perlin_noise()` is 1D and returns about −1..1.

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
