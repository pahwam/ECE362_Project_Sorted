
#ifdef WIFI_DIAG_STANDALONE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/uart.h"

// ESP-01 UART connections
#define WIFI_UART       uart1
#define WIFI_TX_PIN     36
#define WIFI_RX_PIN     37
#define WIFI_BAUD       115200

// Proton onboard red LED
#define LED_PIN         22

#define BUFFER_SIZE     128

static char command[BUFFER_SIZE];
static int cmd_index = 0;

// Send AT command to ESP-01
void wifi_send(const char *cmd) {
    printf("\r\nSending: %s\r\n", cmd);
    uart_puts(WIFI_UART, cmd);
    uart_puts(WIFI_UART, "\r\n");
}

// Change UART baud rate for testing
void wifi_change_baud(uint32_t baud) {
    uart_set_baudrate(WIFI_UART, baud);
    printf("\r\nESP-01 baud set to %lu\r\n",
           (unsigned long)baud);
    printf("Now type AT again.\r\n");
}

// Process commands entered on Mac terminal
void wifi_command(char *cmd) {

    if (strcmp(cmd, "HELP") == 0) {
        printf("\r\n--- AVAILABLE COMMANDS ---\r\n");
        printf("AT             : Test communication\r\n");
        printf("AT+GMR         : Firmware version\r\n");
        printf("AT+CWMODE?     : Check Wi-Fi mode\r\n");
        printf("AT+CWLAP       : Scan Wi-Fi networks\r\n");
        printf("AT+CWJAP?      : Check Wi-Fi connection\r\n");
        printf("BAUD 9600      : Set UART to 9600\r\n");
        printf("BAUD 115200    : Set UART to 115200\r\n");
        printf("HELP           : Display commands\r\n");
    }
    else if (strncmp(cmd, "BAUD ", 5) == 0) {
        char *end;
        unsigned long baud = strtoul(cmd + 5, &end, 10);

        if (*end == '\0' &&
            (baud == 9600 || baud == 57600 ||
             baud == 115200 || baud == 74880)) {
            wifi_change_baud((uint32_t)baud);
            gpio_put(LED_PIN, 0);
        }
        else {
            printf("Unsupported baud rate\r\n");
        }
    }
    else {
        // Forward commands directly to ESP-01
        wifi_send(cmd);
    }
}

int main(void) {

    // Initialize USB terminal
    stdio_init_all();

    // Initialize onboard LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Initialize ESP-01 UART
    uart_init(WIFI_UART, WIFI_BAUD);

    gpio_set_function(WIFI_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(WIFI_RX_PIN, GPIO_FUNC_UART);

    uart_set_format(WIFI_UART, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(WIFI_UART, false, false);
    uart_set_fifo_enabled(WIFI_UART, true);

    bool terminal_connected = false;
    char previous_char = '\0';

    while (true) {

        // Print instructions whenever Mac connects
        bool connected = stdio_usb_connected();

        if (connected && !terminal_connected) {
            printf("\r\n");
            printf("================================\r\n");
            printf(" QUALITYGATE ESP-01 DIAGNOSTIC\r\n");
            printf("================================\r\n");
            printf("ESP-01 TX -> Proton GP37\r\n");
            printf("ESP-01 RX -> Proton GP36\r\n");
            printf("ESP-01 baud: 115200\r\n");
            printf("Type HELP for commands\r\n");
            printf("--------------------------------\r\n");

            // Automatically test communication
            wifi_send("AT");
        }

        if (!connected && terminal_connected) {
            cmd_index = 0;
        }

        terminal_connected = connected;

        // Read responses from ESP-01
        while (uart_is_readable(WIFI_UART)) {

            char c = (char)uart_getc(WIFI_UART);

            // Display ESP response on Mac
            if (connected) {
                putchar_raw(c);
            }

            // Light onboard LED when "OK" received
            if (previous_char == 'O' && c == 'K') {
                gpio_put(LED_PIN, 1);
            }

            previous_char = c;
        }

        // Read commands from Mac terminal
        int received = getchar_timeout_us(0);

        if (received != PICO_ERROR_TIMEOUT) {

            char c = (char)received;

            if (c == '\r' || c == '\n') {

                if (cmd_index > 0) {
                    command[cmd_index] = '\0';
                    wifi_command(command);
                    cmd_index = 0;
                }
            }
            else if (c == '\b' || c == 127) {
                if (cmd_index > 0) {
                    cmd_index--;
                }
            }
            else if (c >= 32 && c <= 126) {
                if (cmd_index < BUFFER_SIZE - 1) {
                    command[cmd_index++] = c;
                }
            }
        }

        sleep_ms(1);
    }
}

#endif // WIFI_DIAG_STANDALONE
