#ifndef COLOR_SENSOR_H
#define COLOR_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "qualitygate_types.h"

#define QG_SPECTRAL_CHANNELS 8

// Stores the eight AS7341 spectral readings
typedef struct {
    uint16_t channel[QG_SPECTRAL_CHANNELS];
} qg_spectrum_t;

// Initialize the color classification system
void color_sensor_init(void);

// Save a reference reading for a known color
bool color_sensor_calibrate(qg_color_t color,
                            const qg_spectrum_t *sample);

// Identify the color of a measured object
qg_color_t color_sensor_classify(
    const qg_spectrum_t *sample);

#endif
