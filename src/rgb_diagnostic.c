#include "pico/stdlib.h"

int main(void) {
    const uint pins[] = {22, 38, 39, 40};

    for (int i = 0; i < 4; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
    }

    while (true) {
        for (int i = 0; i < 4; i++)
            gpio_put(pins[i], 1);

        sleep_ms(1500);

        for (int i = 0; i < 4; i++)
            gpio_put(pins[i], 0);

        sleep_ms(700);
    }
}
