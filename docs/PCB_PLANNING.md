# QualityGate 2.1 — Firmware to PCB planning

We will design the **custom PCB in KiCad** once the exact modules and required voltages are confirmed. **PlatformIO is for compiling/flashing code; it does not design PCBs.**

The first PCB can be a carrier / connector board for the **Proton development board**, rather than trying to re-create all RP2350 circuitry on the first design. This is the lower-risk project approach.

## Known board-reserved functions

| Signal | Proton pin | Note |
| --- | --- | --- |
| User red LED | GP22 | Stage 1 status indicator |
| User yellow LED | GP23 | Available onboard LED |
| User green LED | GP24 | Available onboard LED |
| User blue LED | GP25 | Available onboard LED |
| Button 1 | GP21 | Existing hardware button |
| Button 2 | GP26 | Existing hardware button |
| Lab debugger serial UART | GP0/GP1 | Leave reserved until serial plan is decided |

## Proposed external connections (NOT assigned yet)

| Module | Interface | Pin assignment |
| --- | --- | --- |
| AS7341 spectral sensor | I2C | TBD |
| Break-beam | Digital GPIO | TBD |
| Trapdoor feedback servo | PWM + ADC | TBD |
| Chute feedback servo | PWM + ADC | TBD |
| TFT display | SPI + control GPIO | TBD |
| Bluetooth module | UART | TBD |
| External servo power | Separate regulated supply | Size after servo model/current specs |

## Electrical checklist before drawing PCB

- Confirm all breakout board/module model numbers and supply voltage ratings.
- RP2350 GPIO signals must remain 3.3 V compatible; level-shift if necessary.
- Confirm servo feedback voltage before connecting it to an ADC input.
- Size the servo supply for its stall current; don't power motors directly from microcontroller GPIO.
- Share a ground reference between the servo supply and Proton when appropriate.
- Add test points, connectors and labeling for debug and hardware bring-up.

**Do not fabricate a PCB using this draft pin table**; these pins are not finalized.
