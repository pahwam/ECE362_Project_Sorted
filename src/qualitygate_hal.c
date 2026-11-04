#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "qualitygate_config.h"
#include "qualitygate_hal.h"
#include "qualitygate_pins.h"

#if !QG_SIMULATION_MODE
#error "Real QualityGate hardware driver is not implemented yet. Leave QG_SIMULATION_MODE=1."
#endif

/* For the simulation, a virtual object arrives periodically.
 * It cycles through red, green, blue, yellow, then repeats. */
static bool s_virtual_object_present = false;
static qg_color_t s_virtual_color = QG_COLOR_RED;
static uint32_t s_next_object_ms = 1500;
static uint32_t s_now_ms = 0;
static unsigned s_object_index = 0;

static bool deadline_reached(uint32_t now_ms, uint32_t deadline_ms) {
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

const char *qg_color_name(qg_color_t color) {
    switch (color) {
        case QG_COLOR_RED: return "RED";
        case QG_COLOR_GREEN: return "GREEN";
        case QG_COLOR_BLUE: return "BLUE";
        case QG_COLOR_YELLOW: return "YELLOW";
        default: return "UNKNOWN";
    }
}

const char *qg_state_name(qg_state_t state) {
    switch (state) {
        case QG_WAITING: return "WAITING";
        case QG_SCANNING: return "SCANNING";
        case QG_POSITIONING_CHUTE: return "POSITIONING_CHUTE";
        case QG_DROPPING: return "DROPPING";
        case QG_RESETTING: return "RESETTING";
        case QG_FAULT: return "FAULT";
        default: return "INVALID";
    }
}

void qg_hal_init(void) {
    stdio_init_all();

    const uint led_pins[] = {
        QG_LED_RED_PIN,
        QG_LED_YELLOW_PIN,
        QG_LED_GREEN_PIN,
        QG_LED_BLUE_PIN
    };
    for (unsigned i = 0; i < 4; ++i) {
        gpio_init(led_pins[i]);
        gpio_set_dir(led_pins[i], GPIO_OUT);
        gpio_put(led_pins[i], 0);
    }
    printf("QualityGate HAL: SIMULATION MODE; motors/sensors not connected\n");
}

void qg_hal_poll(uint32_t now_ms) {
    s_now_ms = now_ms;
    if (!s_virtual_object_present &&
        deadline_reached(now_ms, s_next_object_ms)) {
        s_virtual_object_present = true;
        s_virtual_color = (qg_color_t)(s_object_index % QG_COLOR_BIN_COUNT);
        printf("[SIM] Object detected (%s)\n", qg_color_name(s_virtual_color));
    }
}

bool qg_hal_object_present(void) {
    return s_virtual_object_present;
}

qg_color_t qg_hal_read_color(void) {
    return s_virtual_object_present ? s_virtual_color : QG_COLOR_UNKNOWN;
}

void qg_hal_move_chute(qg_color_t color) {
    printf("[SIM] Move chute to %s bin\n", qg_color_name(color));
}

void qg_hal_set_trapdoor(bool open) {
    printf("[SIM] Trapdoor %s\n", open ? "OPEN" : "CLOSED");
    if (open && s_virtual_object_present) {
        s_virtual_object_present = false;
        ++s_object_index;
        s_next_object_ms = s_now_ms + 2000u;
    }
}

void qg_hal_indicate_color(qg_color_t color) {
    gpio_put(QG_LED_RED_PIN, color == QG_COLOR_RED);
    gpio_put(QG_LED_YELLOW_PIN, color == QG_COLOR_YELLOW);
    gpio_put(QG_LED_GREEN_PIN, color == QG_COLOR_GREEN);
    gpio_put(QG_LED_BLUE_PIN, color == QG_COLOR_BLUE);
}
