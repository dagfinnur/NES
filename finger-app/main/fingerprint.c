#include "fingerprint.h"

uint16_t returnResponseCode();
uint32_t returnParameter();

void int32_to_param(uint32_t value, char *param_arr)
{
    param_arr[0] = value & 0xff;
    param_arr[1] = (value >> 8) & 0xff;
    param_arr[2] = (value >> 16) & 0xff;
    param_arr[3] = (value >> 24) & 0xff;
}

void init_uart(void)
{
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
}

void configure_uart(int baudrate, int rx_pin, int tx_pin)
{
    const uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void print_command()
{
    printf("Resp start code 1: %02x\n", cmd_buffer[0]);
    printf("Resp start code 2: %02x\n", cmd_buffer[1]);
    printf("Device id: %02x", (cmd_buffer[2]));
    printf("%02x\n", (cmd_buffer[3]));
    printf("Parameter: %02x", (cmd_buffer[4]));
    printf("%02x", (cmd_buffer[5]));
    printf("%02x", (cmd_buffer[6]));
    printf("%02x\n", (cmd_buffer[7]));
    printf("Command: %02x", (cmd_buffer[8]));
    printf("%02x\n", (cmd_buffer[9]));
    printf("Checksum: %02x", (cmd_buffer[10]));
    printf("%02x\n", (cmd_buffer[11]));

    fflush(stdout);
}

// Function to print the 12 bytes of a response packet
void print_response()
{
    printf("Resp start code 1: %02x\n", resp_buffer[0]);
    printf("Resp start code 2: %02x\n", resp_buffer[1]);
    printf("Device id: %02x", (resp_buffer[3]));
    printf("%02x\n", (resp_buffer[2]));
    printf("Parameter: %02x", (resp_buffer[7]));
    printf("%02x", (resp_buffer[6]));
    printf("%02x", (resp_buffer[5]));
    printf("%02x\n", (resp_buffer[4]));
    printf("Response: %02x", (resp_buffer[9]));
    printf("%02x\n", (resp_buffer[8]));
    printf("Checksum: %02x", (resp_buffer[11]));
    printf("%02x\n", (resp_buffer[10]));

    fflush(stdout);
}

void write_uart()
{

    const int len = 12;
    const int txBytes = uart_write_bytes(UART_NUM_1, cmd_buffer, len);
    // print_command(data);
    printf("*** Sent %d bytes ***\n", txBytes);
    return;
}

int16_t read_uart(uint16_t bytes)
{
    const int rxBytes = uart_read_bytes(UART_NUM_1, resp_buffer, bytes, (TIME_PER_BYTE_MS * bytes));
    // printf("### Read %d bytes ###\n", rxBytes);

    return (uint16_t)rxBytes;
}

uint16_t Checksum(char *packet, int packet_len)
{
    uint16_t sum = 0;

    for (int i = 0; i < packet_len; i++)
    {
        sum += packet[i];
    }

    return sum;
};

void InitFingerprint(void)
{
    cmd_buffer = (char *)malloc(COMMAND_PACKET_LENGTH * sizeof(char));
    resp_buffer = (char *)malloc((RX_BUF_SIZE + 1) * sizeof(char));
};

void ExitFingerprint(void)
{
    free(cmd_buffer);
    free(resp_buffer);
};

void ConstructCommandPacket(uint32_t param, uint16_t cmd)
{
    cmd_buffer[0] = COMMAND_START_CODE_1;
    cmd_buffer[1] = COMMAND_START_CODE_2;

    cmd_buffer[2] = DEVICE_ID_1;
    cmd_buffer[3] = DEVICE_ID_2;

    char *param_arr = (char *)malloc(sizeof(char *) * 4);
    int32_to_param(param, param_arr);

    cmd_buffer[4] = param_arr[0];
    cmd_buffer[5] = param_arr[1];
    cmd_buffer[6] = param_arr[2];
    cmd_buffer[7] = param_arr[3];
    free(param_arr);

    cmd_buffer[8] = cmd & 0xff;
    cmd_buffer[9] = (cmd >> 8) & 0xff;

    uint16_t checksum = Checksum(cmd_buffer, 10);

    cmd_buffer[10] = checksum & 0xff;
    cmd_buffer[11] = (checksum >> 8) & 0xff;

    return;
};

void SendCommand(uint32_t params, char cmd)
{
    ConstructCommandPacket(params, cmd);

    // Send command
    write_uart();

    // Read command
    ReadResponse(COMMAND_PACKET_LENGTH);
}

void ReadResponse(uint32_t bytes)
{
    int16_t bytesRead = read_uart(bytes);

    // Get response
    int16_t response_code = returnResponseCode();

    printf("Response code was 0x%04x\n", response_code);
    fflush(stdout);

    // If the response code is -1, something went wrong when readning the bytes
    if (response_code == -1)
    {
        printf("Something went wrong reading bytes from UART\n");
        fflush(stdout);
    }

    // Response code was Negative-acknowledgement
    if (response_code == CMD_NACK)
    {
        printf("Got NACK\n");
        fflush(stdout);
    }

    // Reponse code acknowledgement
    if (response_code == CMD_ACK)
    {
        // Handle the response
        // print_response();
    }
}

void ReadDataUsingBuffer()
{
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1, (size_t *)&length));
    printf("Initial rxbuffer: %d\n", length);
        fflush(stdout);
    while (length > 0)
    {
        printf("rxbuffer: %d\n", length);
        fflush(stdout);
        int bytes_read = read_uart(length);
        if (bytes_read < length)
        {
            printf("FUCK\n");
            fflush(stdout);
        }
        else
        {
            for (int16_t i = 0; i < bytes_read; i++)
            {
                printf("0x%02x ", resp_buffer[i]);
            }
        }
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1, (size_t *)&length));
        printf("new rxbuffer: %d\n", length);
        fflush(stdout);
    }
    // printf("rxbuffer: %d\n", length);
    // fflush(stdout);
    // int bytes_read = read_uart(length);
    // if (bytes_read < length)
    // {
    //     printf("FUCK\n");
    //     fflush(stdout);
    // }
    // else
    // {
    //     for (int16_t i = 0; i < bytes_read; i++)
    //     {
    //         printf("0x%02x ", resp_buffer[i]);
    //     }
    // }
}

void ReadData(uint16_t size)
{
    // // Split the read into smaller reads of sizes of 1024 bytes
    // uint16_t read_size = 1024;
    // uint16_t number_of_reads = size / read_size;
    // uint16_t remaining_bytes = size - (read_size * number_of_reads);
    // int16_t bytes_read;

    // for (int i = 0; i < number_of_reads; i++)
    // {
    //     bytes_read = read_uart(read_size);
    //     if (bytes_read != read_size)
    //     {
    //         printf("ah shit read %d \n", bytes_read);
    //         fflush(stdout);
    //         return;
    //     }
    //     for (int16_t k = 0; k < bytes_read; k++)
    //     {
    //         printf("0x%02x ", resp_buffer[k]);
    //     }
    // }

    // if (remaining_bytes > 0)
    // {
    //     bytes_read = read_uart(read_size);
    //     if (bytes_read != remaining_bytes)
    //     {
    //         printf("ah shit read %d \n", bytes_read);
    //         fflush(stdout);
    //         return;
    //     }
    //     for (int16_t k = 0; k < bytes_read; k++)
    //     {
    //         printf("0x%02x ", resp_buffer[k]);
    //     }
    // }

    int16_t bytes_read = read_uart(size);
    if (bytes_read < size)
    {
        printf("FUCK\n");
        fflush(stdout);
    }
    else
    {
        for (int16_t i = 0; i < bytes_read; i++)
        {
            printf("0x%02x ", resp_buffer[i]);
        }
    }

    printf("\n");
    fflush(stdout);
}

void Open(void)
{
    SendCommand(0x00, CMD_OPEN);
}

void Close(void)
{
    SendCommand(0x00, CMD_CLOSE);
}

void LedOn(void)
{
    SendCommand(0x01, CMD_CMOS_LED);
}

void LedOff(void)
{
    SendCommand(0x00, CMD_CMOS_LED);
}

void ChangeBaudrate(uint32_t baudrate)
{
    if (baudrate < 9600 || baudrate > 115200)
    {
        printf("Unsupported baudrate specified %ld\n", baudrate);
        return;
    }

    SendCommand(baudrate, CMD_CHANGE_BAUDRATE);
}

void GetEnrolledCount(void)
{
    SendCommand(0x00, CMD_GET_ENROLL_COUNT);
    // print_response();
    if (returnResponseCode() == CMD_ACK)
    {
        printf("Number of enrolled fingerprints: %ld\n", returnParameter());
    }
    else
    {
        printf("Failed to get Enroll Count\n");
        fflush(stdout);
        return;
    }
}

void GetImage(void)
{
    SendCommand(0x00, CMD_GET_IMAGE);
    if (returnResponseCode() == CMD_ACK)
    {
        ReadData(52216);
    }
}

void GetRawImage(void)
{
    SendCommand(0x00, CMD_GET_RAW_IMAGE);
    if (returnResponseCode() == CMD_ACK)
    {
        ReadData(19200);
    }
}

void GetTemplate(uint8_t id)
{
    SendCommand((uint32_t)id, CMD_GET_TEMPLATE);
    if (returnResponseCode() == CMD_ACK)
    {
        printf("ok\n");
        fflush(stdout);
        // ReadData(498);
        ReadDataUsingBuffer();
    }
}

uint32_t returnParameter()
{
    uint32_t param = 0;
    for (int i = 0; i < 4; i++)
    {
        param += resp_buffer[4 + i] << (8 * i);
    }
    return param;
}

uint16_t returnResponseCode()
{
    return ((resp_buffer[9] << 8)) + resp_buffer[8];
}