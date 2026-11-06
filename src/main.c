/*
 * QualityGate 2.1 | Purdue Proton RP2350
 * All-in-one LCD + two-SG90 servo integration test.
 *
 * LCD pins: GP6-13, GP14, GP15, GP18, GP19, GP20
 * Trapdoor servo: GP16  |  Chute servo: GP17
 *
 * Servo power: suitable external 5-6V supply, common GND with Proton.
 * The L298N is ONLY a power distribution point; NOT a servo driver.
 *
 * TEST MODE: automatic movements; no color sensor reads yet.
 * Remove horns / disconnect mechanisms until angles are calibrated.
 */

 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "display.h"
 #include "servo1.h"
 #include "servo2.h"
 
 // Change these angles once the mechanism has been calibrated.
 #define TRAP_CLOSED  45
 #define TRAP_OPEN   115
 #define CHUTE_HOME  90
 
 #define BIN_RED      30
 #define BIN_GREEN    70
 #define BIN_BLUE    110
 #define BIN_YELLOW  150
 
 // Timing in milliseconds
 #define CHUTE_SETTLE_MS  800
 #define TRAP_DROP_MS     900
 #define TRAP_CLOSE_MS    600
 
 // Show live status beneath the original greeting, without redrawing it.
 static void show_status(const char *phase, const char *detail,
                         uint16_t accent) {
     lcd_rect(0, 235, WIDTH, HEIGHT - 235, BLACK);
     draw_text(6, 242, "QUALITYGATE 2.1", WHITE, 1);
     draw_text(6, 262, phase, accent, 1);
     draw_text(6, 282, detail, WHITE, 1);
 }
 
 // Verify GP16 works on its own while GP17 remains at its home angle.
 static void test_trapdoor(void) {
     show_status("TEST: GP16", "TRAP -> CLOSED", CYAN);
     servo1_write(TRAP_CLOSED);
     sleep_ms(1000);
 
     show_status("TEST: GP16", "TRAP -> OPEN", YELLOW);
     servo1_write(TRAP_OPEN);
     sleep_ms(1000);
 
     show_status("TEST: GP16", "TRAP -> CLOSED", CYAN);
     servo1_write(TRAP_CLOSED);
     sleep_ms(1000);
 }
 
 // Verify GP17 works on its own while GP16 stays closed.
 static void test_chute(void) {
     show_status("TEST: GP17", "CHUTE -> 60 DEG", CYAN);
     servo2_write(60);
     sleep_ms(1000);
 
     show_status("TEST: GP17", "CHUTE -> 120 DEG", YELLOW);
     servo2_write(120);
     sleep_ms(1000);
 
     show_status("TEST: GP17", "CHUTE -> HOME", CYAN);
     servo2_write(CHUTE_HOME);
     sleep_ms(1000);
 }
 
 // Demonstrate one object moving through the trapdoor and sorting chute.
 // Chute goes to its destination BEFORE trapdoor opens.
 static void test_sort(const char *color, int chute_angle,
                       uint16_t lcd_color) {
     char status[32];
     snprintf(status, sizeof(status), "BIN: %s", color);
 
     servo1_write(TRAP_CLOSED);
     show_status("TRAP CLOSED", status, lcd_color);
     sleep_ms(300);
 
     servo2_write(chute_angle);
     show_status("CHUTE POSITION", status, lcd_color);
     sleep_ms(CHUTE_SETTLE_MS);
 
     servo1_write(TRAP_OPEN);
     show_status("TRAP OPEN", status, lcd_color);
     sleep_ms(TRAP_DROP_MS);
 
     servo1_write(TRAP_CLOSED);
     show_status("TRAP CLOSED", status, lcd_color);
     sleep_ms(TRAP_CLOSE_MS);
 
     servo2_write(CHUTE_HOME);
     show_status("CHUTE HOME", status, lcd_color);
     sleep_ms(CHUTE_SETTLE_MS);
 }
 
 int main(void) {
     stdio_init_all();
     sleep_ms(500);
 
     // Original SPFD5408 initialization and greeting: unchanged.
     lcd_gpio_init();
     lcd_init();
     lcd_fill(BLACK);
     draw_text(6, 120, "fuck yeah,", RED, 2);
     draw_text(48, 160, "lets goo", CYAN, 2);
     show_status("STARTING", "INITIALIZING SERVOS", CYAN);
 
     // Uses the working individual header logic on GP16 and GP17.
     // Both pins are channels of the same PWM slice (50 Hz).
     servo1_attach();
     servo2_attach();
     servo1_write(TRAP_CLOSED);
     servo2_write(CHUTE_HOME);
     sleep_ms(1000);
 
     // First check each motor independently, one at a time.
     test_trapdoor();
     test_chute();
 
     // Then repeat the full simulated sorting sequence.
     // No AS7341 color classification occurs in this test firmware.
     while (true) {
         test_sort("RED", BIN_RED, RED);
         sleep_ms(800);
         test_sort("GREEN", BIN_GREEN, GREEN);
         sleep_ms(800);
         test_sort("BLUE", BIN_BLUE, BLUE);
         sleep_ms(800);
         test_sort("YELLOW", BIN_YELLOW, YELLOW);
         sleep_ms(800);
     }
 }
 