/* QualityGate: working RGB/sensor diagnostic plus the parallel SPFD5408 TFT.
 * Core 1 owns I2C; core 0 owns the LED, TFT and USB console.
 * Existing wiring is unchanged. GP16/17 are held low: no servo PWM.
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "sensor_rgb.h"
#include "sensor_screen.h"
#include "display.h"

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);

    lcd_gpio_init();
    lcd_init();
    lcd_fill(BLACK);

    draw_text(8, 12, "QUALITYGATE", CYAN, 2);
    draw_text(8, 38, "Sensor readings", WHITE, 1);
    draw_text(8, 56, "LED command: RED", CYAN, 1);

    /* Keep servo signal pins inactive for this diagnostic. */
    gpio_init(16);
    gpio_put(16, 0);
    gpio_set_dir(16, GPIO_OUT);
    gpio_init(17);
    gpio_put(17, 0);
    gpio_set_dir(17, GPIO_OUT);

    /* Sensor reads can continue while the TFT initializes. */
    sensor_rgb_start();
    screen_start();
    printf("\nQualityGate: breathing RGB + APDS9960 + parallel TFT\n");
    printf("Sensor GP4/5; RGB GP38/39/40; servo signals inactive.\n");

    const char *names[4] = {"RED", "GREEN", "BLUE", "YELLOW"};
    uint32_t cycle = 0;
    while (true) {
        ++cycle;
        gpio_put(HEARTBEAT_PIN, (cycle & 1u) != 0);
        for (int color = 0; color < 4; ++color) {
            sensor_state_t before_off = get_sensor();
            set_rgb(0, 0, 0);
            sleep_ms(450);
            sensor_state_t off = get_sensor();

            /* Preserve the successful test's fade and sample timing. */
            for (uint16_t v = 0; v <= BRIGHTNESS; v += 4) {
                show_color(color, v);
                sleep_ms(15);
            }
            show_color(color, BRIGHTNESS);
            sleep_ms(500);
            sensor_state_t lit = get_sensor();
            bool valid = sensor_pair_valid(before_off, off, lit,
                              to_ms_since_boot(get_absolute_time()));

            /* Capture first, draw second. The LED stays at its peak while
             * drawing, so TFT traffic cannot interrupt the fade itself.
             * The displayed data is this captured sample, not the latest
             * acquisition that may arrive while the screen is drawing.
             */
            screen_measurement(names[color], off, lit, valid);
            if (valid) {
                int32_t dr = delta(lit.red, off.red);
                int32_t dg = delta(lit.green, off.green);
                int32_t db = delta(lit.blue, off.blue);
                printf("LED=%s SENSOR=%s ID=0x%02X C=%u R=%u G=%u B=%u\n",
                       names[color], guess_color(dr, dg, db), (unsigned)lit.id,
                       (unsigned)lit.clear, (unsigned)lit.red,
                       (unsigned)lit.green, (unsigned)lit.blue);
                printf("dR=%ld dG=%ld dB=%ld samples=%lu errors=%lu\n",
                       (long)dr, (long)dg, (long)db,
                       (unsigned long)lit.sequence, (unsigned long)lit.errors);
            } else {
                printf("LED=%s SENSOR=NO FRESH DATA ID=0x%02X "
                       "samples=%lu errors=%lu\n", names[color], (unsigned)lit.id,
                       (unsigned long)lit.sequence, (unsigned long)lit.errors);
            }
            sleep_ms(250);

            for (int v = (int)BRIGHTNESS; v >= 0; v -= 4) {
                show_color(color, (uint16_t)v);
                sleep_ms(15);
            }
            set_rgb(0, 0, 0);
            sleep_ms(250);
        }
    }
}
