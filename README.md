# QualityGate 2.1 — ECE 362 Proton (RP2350B)

## Stage 2: Sorting control firmware — hardware simulation

This project uses **C + Pico SDK + PlatformIO**, based on the Purdue Proton ECE 362 lab project configuration. The firmware is structured so the **state machine** remains independent of the real sensor/servo drivers.

**Important:** At this version only the onboard LEDs are real outputs. Color sensing, break-beam detection, chute movement and trapdoor are **simulated**. No physical objects are sorted, and simulated delays are not mechanically verified. We are deliberately postponing hardware verification, as requested.

## Open in VS Code

1. Extract the ZIP, then in VS Code use **File → Open Folder** to open the inner `QualityGate-2.1` folder containing `platformio.ini`.
2. Use the PlatformIO extension as for ECE 362 labs.
3. Coding can continue without the board connected. When you're ready for hardware tests, **Build**, **Upload** with the lab's Picoprobe setup, and use the serial monitor (115200 baud).

## File map

- `platformio.ini`: Purdue Proton/Pico SDK environment. Leave unchanged.
- `src/main.c`: calls hardware initialization then regularly advances the sorter.
- `src/sorter.c` + `include/sorter.h`: non-blocking sorter state machine, safety hold for unknown colors, and bin counts.
- `src/qualitygate_hal.c` + `include/qualitygate_hal.h`: temporary simulated detection/spectral color/servo actions. This will be replaced with real drivers one subsystem at a time.
- `include/qualitygate_types.h`: shared enum types for states and colors.
- `include/qualitygate_config.h`: current simulation mode and (unverified) motion timing constants.
- `include/qualitygate_pins.h`: the Proton's **verified onboard LED GPIOs** only. Don't invent PCB signals.
- `docs/PCB_PLANNING.md`: preliminary PCB planning and power checks.

## State machine

```text
WAITING --part detected--> SCANNING --valid color--> POSITIONING_CHUTE
                            |                           |
                       unknown color                    | settle time
                            v                           v
                          FAULT                       DROPPING
                       (door closed)                   |
                                                  close door/count
                                                      |
                                                      v
                                                  RESETTING
                                                      |
                                            part gone + reset time
                                                      |
                                                      v
                                                    WAITING
```

Virtual parts are generated in the sequence **RED, GREEN, BLUE, YELLOW**. Expected serial monitor includes `[SIM] Object detected (RED)`, state changes and `[COUNTS]` lines. The matching built-in LED lights during each simulated sort.

## Next coding stages

1. **AS7341 I2C driver** for spectral readings, light calibration and robust classification.
2. **Break-beam digital input** with an object-present debounce and jam timeout.
3. **Two feedback-servos** with PWM, ADC readings and travel-limit interlocks. Servos need a suitable separate power supply; all Proton IO must stay 3.3 V compatible.
4. **SPI TFT driver** after controller/pinout verification.
5. **UART Bluetooth** after module/power/voltage verification; keep GP0/GP1 reserved for the lab debugger console.
6. Revisit **PCB schematic** in KiCad with a verified pin map, connectors, common ground, power distribution and test points.

Reference hardware docs: https://ece362-purdue.github.io/proton-labs/
