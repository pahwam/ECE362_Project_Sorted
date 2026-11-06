#ifndef QUALITYGATE_SENSOR_SCREEN_H
#define QUALITYGATE_SENSOR_SCREEN_H

#include <stdio.h>
#include <string.h>
#include "display.h"
#include "sensor_rgb.h"

/* Keep display.h's proven initialization, pinout, font and bus timing.
 * This helper sends a whole opaque character through ONE LCD window,
 * rather than using one lcd_rect() call for every illuminated font pixel.
 */
static void screen_char(int x, int y, char c, uint16_t ink,
                        int scale) {
    unsigned char ch = (unsigned char)c;
    if (ch < 32 || ch > 126) ch = '?';
    int w = 6 * scale, h = 8 * scale;
    if (scale < 1 || x < 0 || y < 0 || x + w > WIDTH || y + h > HEIGHT)
        return;

    lcd_reg(0x0050, (uint16_t)x);
    lcd_reg(0x0051, (uint16_t)(x + w - 1));
    lcd_reg(0x0052, (uint16_t)y);
    lcd_reg(0x0053, (uint16_t)(y + h - 1));
    lcd_reg(0x0020, (uint16_t)x);
    lcd_reg(0x0021, (uint16_t)y);
    gpio_put(LCD_CS, 0);
    gpio_put(LCD_RS, 0);
    write16(0x0022);
    gpio_put(LCD_RS, 1);

    for (int py = 0; py < h; ++py) {
        for (int px = 0; px < w; ++px) {
            int col = px / scale, row = py / scale;
            bool on = col < 5 && row < 7 &&
                      (font5x7[ch - 32][col] & (1u << row));
            write16(on ? ink : BLACK);
        }
    }
    gpio_put(LCD_CS, 1);
}

#define SCREEN_ROWS 13
#define SCREEN_COLS 38
static char screen_previous[SCREEN_ROWS][SCREEN_COLS];
static uint16_t screen_previous_ink[SCREEN_ROWS];
static bool screen_has_row[SCREEN_ROWS];
static const int screen_y[SCREEN_ROWS] = {
    54, 74, 102, 124, 142, 160, 178, 202, 220, 244, 262, 286, 304
};

/* Fixed cells overwrite old digits and trailing letters without flashing. */
static void screen_line(unsigned row, const char *text, uint16_t ink) {
    if (row >= SCREEN_ROWS) return;
    int scale = row == 1 ? 2 : 1;
    int columns = (WIDTH - 12) / (6 * scale);
    size_t length = strlen(text);
    for (int i = 0; i < columns; ++i) {
        char c = (size_t)i < length ? text[i] : ' ';
        if (!screen_has_row[row] || screen_previous_ink[row] != ink ||
            screen_previous[row][i] != c) {
            screen_char(6 + i * 6 * scale, screen_y[row], c, ink, scale);
            screen_previous[row][i] = c;
        }
    }
    screen_previous_ink[row] = ink;
    screen_has_row[row] = true;
}

static uint16_t screen_color(const char *name) {
    if (strcmp(name, "RED") == 0) return RED;
    if (strcmp(name, "GREEN") == 0) return GREEN;
    if (strcmp(name, "BLUE") == 0) return CYAN; /* Readable on black. */
    if (strcmp(name, "YELLOW") == 0) return YELLOW;
    return WHITE;
}

static void screen_start(void) {
    memset(screen_has_row, 0, sizeof screen_has_row);
    lcd_gpio_init();
    lcd_init();
    lcd_fill(BLACK); /* One slow full-screen clear at startup only. */
    draw_text(6, 10, "QUALITYGATE 2.1", CYAN, 2);
    draw_text(6, 34, "RGB LED -> APDS9960 -> TFT", WHITE, 1);
    screen_line(0, "LED command: starting", WHITE);
    screen_line(1, "WAITING", WHITE);
    screen_line(2, "Measured color appears above", WHITE);
    screen_line(11, "SDA GP4 / SCL GP5", WHITE);
    screen_line(12, "LAST PEAK SAMPLE; USB also active", WHITE);
}

static void screen_measurement(const char *led_name, sensor_state_t off,
                                sensor_state_t lit, bool valid) {
    char text[64];
    snprintf(text, sizeof text, "LED command: %s", led_name);
    screen_line(0, text, WHITE);

    if (valid) {
        int32_t dr = delta(lit.red, off.red);
        int32_t dg = delta(lit.green, off.green);
        int32_t db = delta(lit.blue, off.blue);
        const char *name = guess_color(dr, dg, db);
        screen_line(1, name, screen_color(name));
        screen_line(2, "Detected from measured dR/dG/dB", WHITE);
        snprintf(text, sizeof text, "Clear: %5u", (unsigned)lit.clear);
        screen_line(3, text, WHITE);
        snprintf(text, sizeof text, "Red  : %5u", (unsigned)lit.red);
        screen_line(4, text, WHITE);
        snprintf(text, sizeof text, "Green: %5u", (unsigned)lit.green);
        screen_line(5, text, WHITE);
        snprintf(text, sizeof text, "Blue : %5u", (unsigned)lit.blue);
        screen_line(6, text, WHITE);
        snprintf(text, sizeof text, "dR: %5ld   dG: %5ld", (long)dr, (long)dg);
        screen_line(7, text, WHITE);
        snprintf(text, sizeof text, "dB: %5ld", (long)db);
        screen_line(8, text, WHITE);
    } else {
        screen_line(1, "NO FRESH DATA", YELLOW);
        screen_line(2, "Check sensor power / SDA / SCL", YELLOW);
        screen_line(3, "Clear: -----", WHITE);
        screen_line(4, "Red  : -----", WHITE);
        screen_line(5, "Green: -----", WHITE);
        screen_line(6, "Blue : -----", WHITE);
        screen_line(7, "dR: -----   dG: -----", WHITE);
        screen_line(8, "dB: -----", WHITE);
    }
    snprintf(text, sizeof text, "ID: 0x%02X  Samples: %lu", (unsigned)lit.id,
             (unsigned long)lit.sequence);
    screen_line(9, text, WHITE);
    snprintf(text, sizeof text, "I2C errors: %lu", (unsigned long)lit.errors);
    screen_line(10, text, WHITE);
}

#endif /* QUALITYGATE_SENSOR_SCREEN_H */
