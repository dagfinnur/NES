#include "fingerprint.h"

uint16_t returnResponseCode();
uint32_t returnParameter();

// Function to transform a uint32 into an array
// Result is stored in the array the char pointer, points to
void int32_to_param(uint32_t value, char *param_arr)
{
    param_arr[0] = value & 0xff;
    param_arr[1] = (value >> 8) & 0xff;
    param_arr[2] = (value >> 16) & 0xff;
    param_arr[3] = (value >> 24) & 0xff;
}

// Initialize the UART driver
void init_uart(void)
{
    QueueHandle_t uart_queue;
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 10, &uart_queue, 0);
}

// Configure the UART by providing a baudrate, RX and TX pin
void configure_uart(int baudrate, int rx_pin, int tx_pin)
{
    // Setup UART to use 8 data bits, no parity bits, and one stop bit (8N1)
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

// Function used for debugging of the command packet
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

// Function to send a payload using UART
void write_uart()
{
    // This function is only used for sending command payloads
    const int len = COMMAND_PACKET_LENGTH;
    const int txBytes = uart_write_bytes(UART_NUM_1, cmd_buffer, len);
    return;
}

// Function to read data from UART
// Takes two arguments:
// bytes: how many bytes to read
// cursor: where should the read bytes be places in the response buffer
// Returns the number of bytes read
int16_t read_uart(uint16_t bytes, uint32_t cursor)
{
    const int rxBytes = uart_read_bytes(UART_NUM_1, (resp_buffer + cursor), bytes, (100));

    return (uint16_t)rxBytes;
}

// Checksum is calculated by summartion of the packet bytes
uint16_t Checksum(char *packet, int packet_len)
{
    uint16_t sum = 0;

    for (int i = 0; i < packet_len; i++)
    {
        sum += packet[i];
    }

    return sum;
};

// Allocate the space for the command and response buffers
void InitFingerprint(void)
{
    cmd_buffer = (char *)malloc(COMMAND_PACKET_LENGTH * sizeof(char));
    resp_buffer = (char *)malloc(((RX_BUF_SIZE * 2) + 1) * sizeof(char));
};

// Free the allocated space
void ExitFingerprint(void)
{
    free(cmd_buffer);
    free(resp_buffer);
};

// Construct a Command packet given the parameters and command
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
    // We don't need the param array any more, so free it
    free(param_arr);

    cmd_buffer[8] = cmd & 0xff;
    cmd_buffer[9] = (cmd >> 8) & 0xff;

    uint16_t checksum = Checksum(cmd_buffer, 10);

    cmd_buffer[10] = checksum & 0xff;
    cmd_buffer[11] = (checksum >> 8) & 0xff;

    return;
};

// Function that combines the construction, sending and receiving of a Command packet
void SendCommand(uint32_t params, char cmd)
{
    ConstructCommandPacket(params, cmd);

    // Send command
    write_uart();

    // Read command
    ReadResponse(COMMAND_PACKET_LENGTH);
}

// Function to handle the reading of a reponse packet
void ReadResponse(uint32_t bytes)
{
    int16_t bytesRead = read_uart(bytes, 0);

    // Get response
    int16_t response_code = returnResponseCode();

    // printf("Response code was 0x%04x\n", response_code);
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

// Wrapper for the read_uart
void ReadData(uint16_t size, char cursor)
{
    int16_t bytes_read = read_uart(size, cursor);
    if (bytes_read < size)
    {
        printf("Number of bytes read does not match expectation: expected: %d, read: %d\n", size, bytes_read);
    }

    fflush(stdout);
}

// Return the parameter from the response array as a uint32
uint32_t returnParameter()
{
    uint32_t param = 0;
    for (int i = 0; i < 4; i++)
    {
        param += resp_buffer[4 + i] << (8 * i);
    }
    return param;
}

// Return the reponse code of the reponse array
uint16_t returnResponseCode()
{
    return ((resp_buffer[9] << 8)) + resp_buffer[8];
}


void get_sha256_of_template(uint8_t *output) {
    esp_sha_type_t sha_type = ESP_SHA_SHA256;
    esp_sha_handle_t sha_handle;

    esp_sha_start(sha_type, &sha_handle);
    esp_sha_update(sha_handle, (const uint8_t *)resp_buffer, TEMPLATE_SIZE_BYTES);
    esp_sha_finish(sha_handle, output);
}

// GT511C3 specific functions below

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

uint32_t GetEnrolledCount()
{
    uint32_t count = 0;
    SendCommand(0x00, CMD_GET_ENROLL_COUNT);
    // print_response();
    if (returnResponseCode() == CMD_ACK)
    {
        count = returnParameter();
        printf("Number of enrolled fingerprints: %ld\n", count);
        return count;
    }
    else
    {
        printf("Failed to get Enroll Count\n");
        fflush(stdout);
        return count;
    }
}

void GetImage(void)
{
    SendCommand(0x00, CMD_GET_IMAGE);
    if (returnResponseCode() == CMD_ACK)
    {
        ReadData(52216, 0);
    }
}

void GetRawImage(void)
{
    SendCommand(0x00, CMD_GET_RAW_IMAGE);
    if (returnResponseCode() == CMD_ACK)
    {
        uint16_t size = 19200;
        // Split the read into smaller reads of sizes of 1024 bytes
        uint16_t read_size = RX_BUF_SIZE;
        uint16_t number_of_reads = size / read_size;
        uint16_t remaining_bytes = size - (read_size * number_of_reads);

        for (int i = 0; i < number_of_reads; i++)
        {
            printf("cycle %d \n", i);
            fflush(stdout);
            ReadData(read_size, (read_size * i));
        }

        ReadData(remaining_bytes, (read_size * number_of_reads));

        // Print the data

        for (int i = 0; i < number_of_reads; i++)
        {
            for (int u = 0; u < read_size; u++)
            {
                printf("0x%02x ", resp_buffer[(i * read_size) + u]);
            }
            esp_light_sleep_start();
        }
        for (int u = 0; u < remaining_bytes; u++)
        {
            printf("0x%02x ", resp_buffer[(read_size * read_size) + u]);
        }
        printf("\n");
        fflush(stdout);
    }
}

void GetTemplate(uint8_t id)
{
    SendCommand((uint32_t)id, CMD_GET_TEMPLATE);
    if (returnResponseCode() == CMD_ACK)
    {
        printf("ok\n");
        fflush(stdout);
        ReadData(TEMPLATE_SIZE_BYTES, 0);
    }
}

bool CaptureFingerFast()
{
    SendCommand(0x00, CMD_CAPTURE_FINGER);
    if (returnResponseCode() == CMD_ACK)
    {
        return true;
    }
    else
    {
        printf("Failed to capture finger (fast) \n");
        printf("Error code: %lx\n", returnParameter());
    }

    return false;
}

bool CaptureFingerSlow()
{
    SendCommand(0x01, CMD_CAPTURE_FINGER);
    if (returnResponseCode() == CMD_ACK)
    {
        return true;
    }
    else
    {
        printf("Failed to capture finger (slow) \n");
        printf("Error code: %lx\n", returnParameter());
    }

    return false;
}

bool Identification(uint8_t* id)
{
    bool id_ok = false;
    SendCommand(0x00, CMD_IDENTIFY);
    if (returnResponseCode() == CMD_ACK)
    {
        id = (uint8_t)returnParameter();
        printf("Indentified finger with ID: %ld\n", id);
        return true;
    }
    else
    {
        printf("Fingerprint was not authorized\n");
        return false;
    }
    fflush(stdout);

    return id_ok;
}

bool IsFingerPressed()
{
    bool pressed = false;
    SendCommand(0x00, CMD_IS_PRESS_FINGER);
    if (returnResponseCode() == CMD_ACK)
    {
        uint32_t param = returnParameter();
        if (param == 0)
        {
            printf("Finger is pressed\n");
            pressed = true;
            return pressed;
        }
        else
        {
            printf("Finger is not pressed\n");
            pressed = false;
            return pressed;
        }
    }
    else
    {
        printf("Failed to get FingerPressed\n");
    }

    return pressed;
}

void EnrollStart(uint32_t id)
{
    SendCommand(id, CMD_ENROLL_START);
    if (returnResponseCode() == CMD_ACK)
    {
        printf("Start enroll for ID %ld\n", id);
    }
}

void EnrollFirst()
{
    SendCommand(0x00, CMD_ENROLL_FIRST);
    if (returnResponseCode() == CMD_ACK)
    {
        printf("Enrolled first\n");
    }
    else
    {
        printf("Failed to enroll first\n");
    }
    fflush(stdout);
}

void EnrollSecond()
{
    SendCommand(0x00, CMD_ENROLL_SECOND);
    if (returnResponseCode() == CMD_ACK)
    {
        printf("Enrolled second\n");
    }
    else
    {
        printf("Failed to enroll second\n");
    }
    fflush(stdout);
}

void EnrollThird()
{
    SendCommand(0x00, CMD_ENROLL_THIRD);
    if (returnResponseCode() == CMD_ACK)
    {
        printf("Enrolled third\n");
    }
    else
    {
        printf("Failed to enroll third\n");
    }
    fflush(stdout);
}

void Enroll(uint8_t num)
{
    if (num == 1)
    {
        EnrollFirst();
        return;
    }
    if (num == 2)
    {
        EnrollSecond();
        return;
    }
    if (num == 3)
    {
        EnrollThird();
        return;
    }
}
