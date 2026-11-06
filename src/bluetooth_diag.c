
#ifdef BT_DIAG_STANDALONE

#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"

// Bluetooth module
#define BT_UART       uart0
#define BT_TX_PIN     32
#define BT_RX_PIN     33
#define BT_BAUD       9600

// Proton red onboard LED
#define LED_PIN       22

static bool led_state = false;

void bluetooth_send(const char *msg) {
    uart_puts(BT_UART, msg);
}

void led_control(bool on) {
    led_state = on;
    gpio_put(LED_PIN, on ? 1 : 0);
}

void bluetooth_command(char *cmd) {

    // Convert incoming command to uppercase
    for (int i = 0; cmd[i] != '\0'; i++) {
        cmd[i] = (char)toupper((unsigned char)cmd[i]);
    }

    if (strcmp(cmd, "ON") == 0 || strcmp(cmd, "1") == 0) {
        led_control(true);
        bluetooth_send("LED ON\r\n");
    }
    else if (strcmp(cmd, "OFF") == 0 || strcmp(cmd, "0") == 0) {
        led_control(false);
        bluetooth_send("LED OFF\r\n");
    }
    else if (strcmp(cmd, "STATUS") == 0) {
        bluetooth_send(led_state ? "LED IS ON\r\n" : "LED IS OFF\r\n");
    }
    else if (strcmp(cmd, "PING") == 0) {
        bluetooth_send("PONG - Bluetooth working!\r\n");
    }
    else if (strcmp(cmd, "HELP") == 0) {
        bluetooth_send("ON, OFF, STATUS, PING, HELP\r\n");
    }
    else {
        bluetooth_send("Unknown command\r\n");
    }
}

int main(void) {

    // Initialize LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    led_control(false);

    // Initialize Bluetooth UART
    uart_init(BT_UART, BT_BAUD);

    gpio_set_function(BT_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(BT_RX_PIN, GPIO_FUNC_UART);

    uart_set_format(BT_UART, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(BT_UART, false, false);

    bluetooth_send("QualityGate Bluetooth Ready!\r\n");

    char command[32];
    int index = 0;

    while (true) {

        if (uart_is_readable(BT_UART)) {

            char c = uart_getc(BT_UART);

            if (c == '\n' || c == '\r') {

                if (index > 0) {
                    command[index] = '\0';
                    bluetooth_command(command);
                    index = 0;
                }
            }
            else if (c == '\b' || c == 127) {
                if (index > 0) index--;
            }
            else if (index < 31) {
                command[index++] = c;
            }
        }

        tight_loop_contents();
    }
}

#endif // BT_DIAG_STANDALONE
