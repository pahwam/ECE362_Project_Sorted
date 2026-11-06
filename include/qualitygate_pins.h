#ifndef QUALITYGATE_PINS_H
#define QUALITYGATE_PINS_H

/*
 * QualityGate 2.1 - central source of truth for pin assignments.
 *
 * Confirmed pins ON THE PURDUE PROTON BOARD:
 *   GP22 = red user LED
 *   GP23 = yellow user LED
 *   GP24 = green user LED
 *   GP25 = blue user LED
 *   GP21 = button, GP26 = button
 *
 * GP0/GP1 are used by the usual ECE 362 debugger UART setup.
 * Do not allocate them to the project's Bluetooth module yet.
 *
 * External sensor/servo/display pin assignments are intentionally
 * UNASSIGNED until module models, wiring and voltage levels are verified.
 * We will choose them before making the KiCad PCB schematic.
 */

#define QG_LED_RED_PIN       22
#define QG_LED_YELLOW_PIN    23
#define QG_LED_GREEN_PIN     24
#define QG_LED_BLUE_PIN      25

#endif // QUALITYGATE_PINS_H
