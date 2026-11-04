#ifndef QUALITYGATE_CONFIG_H
#define QUALITYGATE_CONFIG_H

/*
 * QUALITYGATE 2.1 — firmware configuration
 *
 * At this stage, the sorting workflow uses SIMULATED parts.
 * No external sensors or motors are driven yet. Leave this at 1.
 * We will implement a real hardware backend once the exact modules and
 * supply voltages have been confirmed, then change the build mode.
 */
#define QG_SIMULATION_MODE 1

/* Timings in milliseconds. These are software-demo values, NOT yet
 * characterized for actual motors, sensors, or mechanical geometry. */
#define QG_SCAN_DELAY_MS       200u
#define QG_CHUTE_SETTLE_MS     600u
#define QG_TRAPDOOR_OPEN_MS    400u
#define QG_RESET_DELAY_MS     300u

#endif /* QUALITYGATE_CONFIG_H */
