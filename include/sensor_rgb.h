#ifndef QUALITYGATE_SENSOR_RGB_H
#define QUALITYGATE_SENSOR_RGB_H
/* Extracted from your working color_sensor_working.h.
 * Keep its blocking I2C pattern and register settings for this bring-up test.
 * Include in main.c only: the shared state and helpers are file-static.
 */
/*
 * QualityGate 2.1: COMMON-ANODE RGB breathing + APDS9960 diagnostic
 * Purdue Proton RP2350, Pico SDK / PlatformIO
 *
 * RGB common anode -> 3V3.  R->330R->GP38, G->330R->GP39, B->330R->GP40.
 * APDS9960: 3V3, GND, SDA GP4, SCL GP5.
 *
 * Core 0: LED PWM + USB serial (always continues)
 * Core 1: APDS9960 using SAME BLOCKING I2C pattern as working sensor-only test
 *          (a sensor stall cannot prevent the LED from changing colors).
 *
 * Requires -DPIO_NO_STDIO_UART in platformio.ini build_flags for USB serial.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

#define SENSOR_BUS i2c0
#define SENSOR_ADDR 0x39
#define SDA_PIN 4
#define SCL_PIN 5
#define RED_PIN 38
#define GREEN_PIN 39
#define BLUE_PIN 40
#define HEARTBEAT_PIN 22

#define PWM_TOP 999u
#define PWM_PERIOD 1000u
#define BRIGHTNESS 120u  // max 12% on common-anode RGB. Try 50 for 5%.

#define REG_ENABLE  0x80
#define REG_ATIME   0x81
#define REG_CONTROL 0x8F
#define REG_ID      0x92
#define REG_STATUS  0x93
#define REG_CDATAL  0x94

typedef struct {
    uint16_t clear, red, green, blue;
    uint8_t id;
    uint32_t sequence;
    uint32_t errors;
    bool initialized;
    uint32_t sample_ms;  // Time of the last completed RGBC read.
} sensor_state_t;

static mutex_t state_mutex;
static sensor_state_t state = {0};

/* This is the same blocking I2C pattern as apds9960_sensor_diagnostic.c. */
static bool read_regs(uint8_t reg, uint8_t *buffer, size_t n) {
    if (i2c_write_blocking(SENSOR_BUS, SENSOR_ADDR, &reg, 1, true) != 1)
        return false;
    return i2c_read_blocking(SENSOR_BUS, SENSOR_ADDR, buffer, n, false) == (int)n;
}

static bool write_reg(uint8_t reg, uint8_t value) {
    uint8_t pair[2] = {reg, value};
    return i2c_write_blocking(SENSOR_BUS, SENSOR_ADDR, pair, 2, false) == 2;
}

static uint16_t as_u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static sensor_state_t get_sensor(void) {
    sensor_state_t snap;
    mutex_enter_blocking(&state_mutex);
    snap = state;
    mutex_exit(&state_mutex);
    return snap;
}

/* Separate core: the LED and console never need to wait for a sensor read. */
static void sensor_core(void) {
    i2c_init(SENSOR_BUS, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    bool configured = false;
    for (;;) {
        if (!configured) {
            uint8_t id = 0;
            if (!read_regs(REG_ID, &id, 1)) {
                mutex_enter_blocking(&state_mutex);
                state.errors++;
                mutex_exit(&state_mutex);
                sleep_ms(500);
                continue;
            }

            bool ok = write_reg(REG_ENABLE, 0x00) &&
                      write_reg(REG_ATIME, 0xDB) &&
                      write_reg(REG_CONTROL, 0x01) &&
                      write_reg(REG_ENABLE, 0x03);
            if (!ok) {
                mutex_enter_blocking(&state_mutex);
                state.errors++;
                mutex_exit(&state_mutex);
                sleep_ms(500);
                continue;
            }
            mutex_enter_blocking(&state_mutex);
            state.id = id; // The user's module previously reported 0x9E.
            state.initialized = true;
            mutex_exit(&state_mutex);
            configured = true;
            sleep_ms(150);
        }

        uint8_t status = 0;
        if (!read_regs(REG_STATUS, &status, 1)) {
            configured = false;
            mutex_enter_blocking(&state_mutex);
            state.initialized = false;
            state.errors++;
            mutex_exit(&state_mutex);
        } else if ((status & 1u) != 0) {
            uint8_t b[8];
            if (read_regs(REG_CDATAL, b, sizeof b)) {
                mutex_enter_blocking(&state_mutex);
                state.clear = as_u16(&b[0]);
                state.red   = as_u16(&b[2]);
                state.green = as_u16(&b[4]);
                state.blue  = as_u16(&b[6]);
                state.sequence++;
                state.sample_ms = to_ms_since_boot(get_absolute_time());
                mutex_exit(&state_mutex);
            } else {
                configured = false;
                mutex_enter_blocking(&state_mutex);
                state.initialized = false;
                state.errors++;
                mutex_exit(&state_mutex);
            }
        }
        sleep_ms(130);
    }
}

/* In common anode, PWM HIGH means OFF. Invert each color's duty. */
static void set_rgb(uint16_t red, uint16_t green, uint16_t blue) {
    pwm_set_gpio_level(RED_PIN, PWM_PERIOD - red);
    pwm_set_gpio_level(GREEN_PIN, PWM_PERIOD - green);
    pwm_set_gpio_level(BLUE_PIN, PWM_PERIOD - blue);
}

static void init_rgb(void) {
    const uint pins[] = {RED_PIN, GREEN_PIN, BLUE_PIN};
    uint slices[3];
    for (int i = 0; i < 3; ++i) {
        gpio_init(pins[i]);
        gpio_put(pins[i], 1); // OFF for common anode
        gpio_set_dir(pins[i], GPIO_OUT);
        slices[i] = pwm_gpio_to_slice_num(pins[i]);
    }

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 125.0f);
    pwm_config_set_wrap(&cfg, PWM_TOP);

    for (int i = 0; i < 3; ++i) {
        bool duplicate = false;
        for (int j = 0; j < i; ++j)
            if (slices[i] == slices[j]) duplicate = true;
        if (!duplicate) pwm_init(slices[i], &cfg, false);
    }

    set_rgb(0, 0, 0);
    for (int i = 0; i < 3; ++i)
        gpio_set_function(pins[i], GPIO_FUNC_PWM);

    for (int i = 0; i < 3; ++i) {
        bool duplicate = false;
        for (int j = 0; j < i; ++j)
            if (slices[i] == slices[j]) duplicate = true;
        if (!duplicate) pwm_set_enabled(slices[i], true);
    }
}

static void show_color(int color, uint16_t amount) {
    set_rgb((color == 0 || color == 3) ? amount : 0,
            (color == 1 || color == 3) ? amount : 0,
            color == 2 ? amount : 0);
}

static int32_t delta(uint16_t lit, uint16_t background) {
    int32_t value = (int32_t)lit - (int32_t)background;
    return value > 0 ? value : 0;
}

static const char *guess_color(int32_t r, int32_t g, int32_t b) {
    if (r + g + b < 30) return "TOO_DIM";
    if (r > b * 13 / 10 && g > b * 13 / 10 &&
        r < g * 3 && g < r * 3) return "YELLOW";
    if (r > g * 14 / 10 && r > b * 14 / 10) return "RED";
    if (g > r * 13 / 10 && g > b * 12 / 10) return "GREEN";
    if (b > r * 13 / 10 && b > g * 12 / 10) return "BLUE";
    return "UNKNOWN";
}


/* Unsigned subtraction also handles the millisecond counter wrapping. */
static bool sensor_sample_fresh(sensor_state_t s, uint32_t now_ms) {
    return s.initialized && s.sequence != 0 &&
           (uint32_t)(now_ms - s.sample_ms) < 1000u;
}

static bool sensor_pair_valid(sensor_state_t before_off,
                              sensor_state_t off,
                              sensor_state_t lit,
                              uint32_t now_ms) {
    return off.initialized && off.sequence != 0 &&
           off.sequence != before_off.sequence &&
           sensor_sample_fresh(lit, now_ms) &&
           lit.sequence != off.sequence && lit.errors == off.errors;
}

static void sensor_rgb_start(void) {
    gpio_init(HEARTBEAT_PIN);
    gpio_put(HEARTBEAT_PIN, 0);
    gpio_set_dir(HEARTBEAT_PIN, GPIO_OUT);
    init_rgb();
    mutex_init(&state_mutex);
    multicore_launch_core1(sensor_core);
}

#endif /* QUALITYGATE_SENSOR_RGB_H */
