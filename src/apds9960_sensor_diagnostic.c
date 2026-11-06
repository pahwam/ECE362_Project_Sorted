/*
 * QualityGate 2.1 — Standalone APDS9960 sensor diagnostic
 * Proton RP2350: GP4 SDA, GP5 SCL, 3V3, GND
 * No RGB LED or servo code.
 *
 * IMPORTANT (custom Proton PlatformIO/PicoSDK fork):
 * platformio.ini must have: build_flags = -DPIO_NO_STDIO_UART
 * This enables USB CDC stdio for printf().
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define SENSOR_I2C i2c0
#define SENSOR_ADDR 0x39
#define SENSOR_SDA  4
#define SENSOR_SCL  5

#define APDS_ENABLE 0x80
#define APDS_ATIME  0x81
#define APDS_CONTROL 0x8F
#define APDS_ID     0x92
#define APDS_STATUS 0x93
#define APDS_CDATAL 0x94

static bool sensor_read(uint8_t reg, uint8_t *buffer, size_t n) {
    if (i2c_write_timeout_us(SENSOR_I2C, SENSOR_ADDR, &reg, 1, true, 50000) != 1)
        return false;
    return i2c_read_timeout_us(SENSOR_I2C, SENSOR_ADDR, buffer, n, false, 50000) == (int)n;
}

static bool sensor_write(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    return i2c_write_timeout_us(SENSOR_I2C, SENSOR_ADDR, buffer, 2, false, 50000) == 2;
}

static uint16_t u16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

int main(void) {
    stdio_init_all();
    sleep_ms(2500);  // USB monitor has time to open
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\n=== QualityGate 2.1 APDS9960 SENSOR-ONLY TEST ===\n");
    printf("I2C0: SDA=GP4  SCL=GP5  address=0x39\n");

    i2c_init(SENSOR_I2C, 100 * 1000);
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    printf("I2C initialized! SDA=%d SCL=%d\n", gpio_get(SENSOR_SDA), gpio_get(SENSOR_SCL));

    uint8_t id = 0;
    while (!sensor_read(APDS_ID, &id, 1)) {
        printf("NO I2C RESPONSE at 0x39. Check 3V3, GND, SDA=GP4, SCL=GP5.\n");
        sleep_ms(1000);
    }
    printf("Sensor responds! ID = 0x%02X (APDS9960 usually 0xAB)\n", id);
    if (id != 0xAB)
        printf("WARNING: Unexpected ID; check module label / chip.\n");

    // ALS/RGBC only; no proximity/gesture and no LED output.
    // ATIME=0xDB => about 101 ms integration time; CONTROL=0x01 => 4x ALS gain.
    bool ok = sensor_write(APDS_ENABLE, 0x00) &&
              sensor_write(APDS_ATIME, 0xDB) &&
              sensor_write(APDS_CONTROL, 0x01) &&
              sensor_write(APDS_ENABLE, 0x03);
    if (!ok) {
        printf("Failed to configure sensor registers.\n");
        while (true) sleep_ms(1000);
    }
    printf("RGBC measurements enabled. Move colored objects near sensor.\n\n");
    sleep_ms(150);

    while (true) {
        uint8_t status = 0;
        if (!sensor_read(APDS_STATUS, &status, 1)) {
            printf("I2C READ ERROR; check wiring.\n");
        } else if ((status & 0x01u) == 0) {
            printf("Waiting for valid RGBC data (status=0x%02X)\n", status);
        } else {
            uint8_t raw[8];
            if (!sensor_read(APDS_CDATAL, raw, sizeof raw)) {
                printf("Could not read RGBC registers.\n");
            } else {
                uint16_t clear = u16le(&raw[0]);
                uint16_t red   = u16le(&raw[2]);
                uint16_t green = u16le(&raw[4]);
                uint16_t blue  = u16le(&raw[6]);
                printf("CLEAR=%5u | RED=%5u | GREEN=%5u | BLUE=%5u\n",
                       (unsigned)clear, (unsigned)red,
                       (unsigned)green, (unsigned)blue);
            }
        }
        sleep_ms(500);
    }
}
