#include <stdio.h>
#include "fingerprint.c"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_14)
#define RXD_PIN (GPIO_NUM_15)

void print_send(char *data)
{
    printf("Resp start code 1: %02x\n", data[0]);
    printf("Resp start code 2: %02x\n", data[1]);
    printf("Device id: %02x", (data[2]));
    printf("%02x\n", (data[3]));
    printf("Parameter: %02x", (data[4]));
    printf("%02x", (data[5]));
    printf("%02x", (data[6]));
    printf("%02x\n", (data[7]));
    printf("Command: %02x", (data[8]));
    printf("%02x\n", (data[9]));
    printf("Checksum: %02x", (data[10]));
    printf("%02x\n", (data[11]));
    printf("====\n");

    fflush(stdout);
}

void write_uart(char *data)
{
    printf("About to send:\n");
    print_send(data);
    const int len = 12; // strlen(data);
    for (int i = 0; i < len; i++){
        printf("%02x", data[i]);
    }
    printf("\n");
    fflush(stdout);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    printf("Sent %d bytes\n", txBytes);
    return;
}

void print_response(uint8_t *data)
{
    printf("Resp start code 1: %02x\n", data[0]);
    printf("Resp start code 2: %02x\n", data[1]);
    printf("Device id: %02x", (data[3]));
    printf("%02x\n", (data[2]));
    printf("Parameter: %02x", (data[7]));
    printf("%02x", (data[6]));
    printf("%02x", (data[5]));
    printf("%02x\n", (data[4]));
    printf("Response: %02x", (data[9]));
    printf("%02x\n", (data[8]));
    printf("Checksum: %02x", (data[11]));
    printf("%02x\n", (data[10]));
    printf("====\n");

    fflush(stdout);
}

void read_uart()
{
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 500);
    printf("bytes read: %d \n", rxBytes);
    if (rxBytes == 12)
    {
        print_response(data);
    }
    else
    {
        data[rxBytes] = 0;
    }
    fflush(stdout);
    free(data);
}

void app_main(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    char *cmd_buffer = malloc(12 * sizeof(char *));

    ConstructCommandPacket(0x00, CMD_OPEN, cmd_buffer);
    write_uart(cmd_buffer);
    read_uart();

    ConstructCommandPacket(0x01, CMD_CMOS_LED, cmd_buffer);
    write_uart(cmd_buffer);
    read_uart();

    ConstructCommandPacket(0x00, CMD_CMOS_LED, cmd_buffer);
    write_uart(cmd_buffer);
    read_uart();

    free(cmd_buffer);
}
