# Hardware

Raspberry Pi Pico, receive-only DMX-512, 16 PWM outputs for bulbs, and one WS2812 stripe. The flame look is not chosen yet.

## Pico pinout

USB is at the top. Pin 1 is GP0. GP25 is the onboard LED, not a header pin. GP23 and GP24 are unused.

```
                              USB
            +--------------------------------------+
 GP0  PWM0  | 1                                 40 | VBUS   5 V for the RS-485 chip
 GP1  PWM1  | 2                                 39 | VSYS
 GND        | 3                                 38 | GND
 GP2  PWM2  | 4                                 37 | 3V3_EN
 GP3  PWM3  | 5                                 36 | 3V3    pots and logic
 GP4  PWM4  | 6                                 35 | ADC_VREF
 GP5  PWM5  | 7                                 34 | GP28   pot 2
 GND        | 8                                 33 | GND
 GP6  PWM6  | 9                                 32 | GP27   pot 1
 GP7  PWM7  | 10                                31 | GP26   pot 0
 GP8  PWM8  | 11                                30 | RUN
 GP9  PWM9  | 12                                29 | GP22   WS2812 data
 GND        | 13                                28 | GND
 GP10 PWM10 | 14                                27 | GP21   DIP bit 0 (LSB)
 GP11 PWM11 | 15                                26 | GP20   DIP bit 1
 GP12 PWM12 | 16                                25 | GP19   DIP bit 2
 GP13 PWM13 | 17                                24 | GP18   DIP bit 3 (MSB)
 GND        | 18                                23 | GND
 GP14 PWM14 | 19                                22 | GP17   DMX RX, from RO
 GP15 PWM15 | 20                                21 | GP16   mode, low = manual
            +--------------------------------------+

 GP25 onboard LED   heartbeat, held on while DMX frames arrive
```

PWM pins are high = on, about 18 kHz, 7388 steps at 133 MHz. DIP switches and the mode switch close to GND. The pins have internal pull-ups. Each pot is wired from 3V3 to GND, wiper to GP26, GP27, or GP28.

Start channel = `1 + dip * 16`. DIP 0 is channels 1..16. DIP 15 is channels 241..256.

## DMX receiver (MAX485 or SN75176)

Both parts use the same 8-pin footprint. This board only receives. DE and /RE stay low, so the driver is off and the receiver is on.

RO on these chips swings to 5 V. GPIO 17 is a 3.3 V pin, so RO goes through a divider. Do not wire RO straight to the Pico.

```
 XLR-5 female                         MAX485 / SN75176
 (DMX in)

 pin 1 shield GND ----+---------------- GND (pin 5) ---- Pico GND
                      |
 pin 2 Data- ---------+------ B (pin 7)
                      |
                     120
                      |
 pin 3 Data+ ---------+------ A (pin 6)
                      |       |
                     680     680
                      |       |
                     GND     +5 V

 /RE (pin 2) ---- GND          receiver enabled
 DE  (pin 3) ---- GND          driver off
 DI  (pin 4) ---- GND          unused
 VCC (pin 8) ---- Pico VBUS    5 V, USB powered
 GND (pin 5) ---- Pico GND

 RO (pin 1) ---- 2k2 ----+---- GPIO 17
                         |
                        3k3
                         |
                        GND
```

The divider is about `5 V * 3.3 / (2.2 + 3.3) = 3.0 V` at the Pico pin.

The 120 ohm resistor terminates the pair. The two 680 ohm resistors idle the pair in the mark state when nothing is driving it: A is pulled up, B is pulled down, so RO rests high. With the terminator fitted, the idle difference is about `5 V * 120 / (680 + 120 + 680) = 0.4 V`, which is enough for the receiver.

VBUS is 5 V only while USB is plugged in. If the Pico is powered some other way, the RS-485 chip still needs its own 5 V, and the grounds stay joined.

## Bulbs (ULN2803)

Sixteen PWM pins need two ULN2803 Darlington arrays, eight channels each. The Pico pin drives the ULN input. A high PWM level turns that Darlington on and the bulb lights. That matches the firmware: high = on.

The ULN outputs are open collector. Each bulb sits between the positive lamp supply and an ULN output. COM (pin 10) of each package goes to that same positive supply so the internal diodes can clamp the wiring. GND (pin 9) goes to Pico GND. The lamp supply and the Pico must share that ground. The lamp positive does not have to be 3.3 V or 5 V.

```
 Pico GP0 .. GP7  ---> ULN2803 A inputs 1..8
 Pico GP8 .. GP15 ---> ULN2803 B inputs 1..8

 lamp +V ----+---- COM (pin 10) of A and of B
             |
             +---- bulb ---- ULN output
             |
            (one bulb per channel)

 ULN GND (pin 9) ---- Pico GND
```

On the ULN2803, output 8 is pin 11 and output 1 is pin 18. Input 1 (pin 1) switches the pin 18 output.

Each channel is rated 500 mA peak. The package cannot hold that on every channel at once. For a DIP package, plan on about 100 mA per bulb when many channels are bright together. Small 12 V lamps fit. A mains bulb does not. The built-in input resistors are sized for 5 V TTL. At 3.3 V the base current is smaller, and it is still enough to switch this kind of load.

## Flame calls per LED

The flame look is not chosen yet. The call rate comes from the main loop, not from which look is filled in later.

Each pass calls `fire` once per PWM output, writes those 16 levels, then sends the stripe. The stripe is 16 colors repeated four times: 64 pixels, 24 bits each, 800 kHz.

```
64 * 24 / 800000 = 1.92 ms
```

That send blocks. Reading the DIP switch, the mode pin, and the three pots, then calling `fire` sixteen times, adds about 0.3 ms. A pass is about 2.2 ms.

```
1 / 2.2 ms ≈ 450 calls per LED per second
```

If `fire` were free, the stripe alone would cap the loop at about 520 calls per second. A visible flame changes more like 25 to 50 times a second, so each LED is recalculated about ten times faster than the flicker you see.
