#ifndef SERVO1_H
#define SERVO1_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define SERVO1_PIN 16  // Trapdoor

// GP16 and GP17 share a PWM slice. Do not reset the slice when
// initializing the second servo: both channels must keep working.
static inline void servo1_attach(void) {
    gpio_set_function(SERVO1_PIN, GPIO_FUNC_PWM);

    uint slice = pwm_gpio_to_slice_num(SERVO1_PIN);
    float divider = (float)clock_get_hz(clk_sys) / 1000000.0f;

    pwm_set_clkdiv(slice, divider);
    pwm_set_wrap(slice, 19999);  // 50 Hz, 1 us per tick
    pwm_set_gpio_level(SERVO1_PIN, 1500); // Center
    pwm_set_enabled(slice, true);
}

static inline void servo1_write(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    uint16_t pulse = (uint16_t)(1000 + angle * 1000 / 180);
    pwm_set_gpio_level(SERVO1_PIN, pulse);
}

#endif
