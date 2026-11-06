#ifndef SERVO2_H
#define SERVO2_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define SERVO2_PIN 17  // Sorting chute

// Sets up channel B of the shared PWM slice without resetting channel A.
static inline void servo2_attach(void) {
    gpio_set_function(SERVO2_PIN, GPIO_FUNC_PWM);

    uint slice = pwm_gpio_to_slice_num(SERVO2_PIN);
    float divider = (float)clock_get_hz(clk_sys) / 1000000.0f;

    pwm_set_clkdiv(slice, divider);
    pwm_set_wrap(slice, 19999);  // 50 Hz, 1 us per tick
    pwm_set_gpio_level(SERVO2_PIN, 1500); // Center
    pwm_set_enabled(slice, true);
}

static inline void servo2_write(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    uint16_t pulse = (uint16_t)(1000 + angle * 1000 / 180);
    pwm_set_gpio_level(SERVO2_PIN, pulse);
}

#endif