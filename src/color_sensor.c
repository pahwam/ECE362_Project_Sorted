
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include "color_sensor.h"

// Reference spectral readings for each color
static uint16_t references[QG_COLOR_BIN_COUNT]
                          [QG_SPECTRAL_CHANNELS];

static bool calibrated[QG_COLOR_BIN_COUNT];

// Normalize readings to reduce brightness differences
static bool normalize(const qg_spectrum_t *sample,
                      uint16_t output[QG_SPECTRAL_CHANNELS]) {

    if (sample == NULL || output == NULL) {
        return false;
    }

    uint32_t total = 0;

    for (int i = 0; i < QG_SPECTRAL_CHANNELS; i++) {
        total += sample->channel[i];
    }

    if (total == 0) {
        return false;
    }

    for (int i = 0; i < QG_SPECTRAL_CHANNELS; i++) {
        output[i] =
            (uint32_t)sample->channel[i] * 1000u / total;
    }

    return true;
}

void color_sensor_init(void) {

    for (int c = 0; c < QG_COLOR_BIN_COUNT; c++) {
        calibrated[c] = false;

        for (int i = 0; i < QG_SPECTRAL_CHANNELS; i++) {
            references[c][i] = 0;
        }
    }

    printf("Color classification initialized\n");
}

// Save a known color's spectral pattern
bool color_sensor_calibrate(qg_color_t color,
                            const qg_spectrum_t *sample) {

    if (color < QG_COLOR_RED ||
        color >= QG_COLOR_BIN_COUNT) {
        return false;
    }

    if (!normalize(sample, references[color])) {
        return false;
    }

    calibrated[color] = true;
    return true;
}

// Compare a sample against calibrated colors
qg_color_t color_sensor_classify(
    const qg_spectrum_t *sample) {

    uint16_t measured[QG_SPECTRAL_CHANNELS];

    if (!normalize(sample, measured)) {
        return QG_COLOR_UNKNOWN;
    }

    // Require calibration for every sorting bin
    for (int c = 0; c < QG_COLOR_BIN_COUNT; c++) {
        if (!calibrated[c]) {
            return QG_COLOR_UNKNOWN;
        }
    }

    uint32_t best_distance = UINT32_MAX;
    uint32_t second_distance = UINT32_MAX;

    qg_color_t result = QG_COLOR_UNKNOWN;

    for (int c = 0; c < QG_COLOR_BIN_COUNT; c++) {

        uint32_t distance = 0;

        for (int i = 0; i < QG_SPECTRAL_CHANNELS; i++) {

            int32_t difference =
                (int32_t)measured[i] - references[c][i];

            distance += difference * difference;
        }

        if (distance < best_distance) {
            second_distance = best_distance;
            best_distance = distance;
            result = (qg_color_t)c;
        }
        else if (distance < second_distance) {
            second_distance = distance;
        }
    }

    // Initial safety thresholds; tune after testing
    if (best_distance > 80000u) {
        return QG_COLOR_UNKNOWN;
    }

    if (second_distance - best_distance < 5000u) {
        return QG_COLOR_UNKNOWN;
    }

    return result;
}
