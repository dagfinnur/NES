#ifndef FINGER_PRINT
#define FINGER_PRINT

#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_sleep.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "mbedtls/sha256.h"

/*
Command and error codes found in the GT-511C3 data sheet v2.1 (04/11/2016)
*/

// Packet sizes
const uint16_t LARGEST_DATA_PACKET_BYTES = 52216;
const uint16_t TEMPLATE_SIZE_BYTES = 498;
const char COMMAND_PACKET_LENGTH = 12;
// Command packet information
const char COMMAND_START_CODE_1 = 0x55;
const char COMMAND_START_CODE_2 = 0xAA;
const char DEVICE_ID_1 = 0x01;
const char DEVICE_ID_2 = 0x00;

// Commands
const char CMD_OPEN = 0x01;
const char CMD_CLOSE = 0x02;
const char CMD_USB_INTERNAL_CHECK = 0x03;
const char CMD_CHANGE_BAUDRATE = 0x04;
const char CMD_SET_IAP_MODE = 0x05;
const char CMD_CMOS_LED = 0x12;
const char CMD_GET_ENROLL_COUNT = 0x20;
const char CMD_CHECK_ENROLLED = 0x21;
const char CMD_ENROLL_START = 0x22;
const char CMD_ENROLL_FIRST = 0x23;
const char CMD_ENROLL_SECOND = 0x24;
const char CMD_ENROLL_THIRD = 0x25;
const char CMD_IS_PRESS_FINGER = 0x26;
const char CMD_DELETE_ID = 0x40;
const char CMD_DELETE_ALL = 0x41;
const char CMD_VERIFY = 0x50;
const char CMD_IDENTIFY = 0x51;
const char CMD_VERIFY_TEMPLATE = 0x52;
const char CMD_IDENTIFY_TEMPLATE = 0x53;
const char CMD_CAPTURE_FINGER = 0x60;
const char CMD_MAKE_TEMPLATE = 0x61;
const char CMD_GET_IMAGE = 0x62;
const char CMD_GET_RAW_IMAGE = 0x63;
const char CMD_GET_TEMPLATE = 0x70;
const char CMD_SET_TEMPLATE = 0x71;
const char CMD_GET_DATABASE_START = 0x72;
const char CMD_GET_DATABASE_END = 0x73;
const char CMD_SET_SECURITY_LEVEL = 0xf0;
const char CMD_GET_SECURITY_LEVEL = 0xf1;
const char CMD_ACK = 0x30;
const char CMD_NACK = 0x31;

// Error codes
const uint16_t NACK_TIMEOUT = 0x1001;
const uint16_t NACK_INVALID_BAUDRATE = 0x1002;
const uint16_t NACK_INVALID_POS = 0x1003;
const uint16_t NACK_IS_NOT_USED = 0x1004;
const uint16_t NACK_IS_ALREADY_USED = 0x1005;
const uint16_t NACK_COMM_ERR = 0x1006;
const uint16_t NACK_VERIFY_FAILED = 0x1007;
const uint16_t NACK_IDENTIFY_FAILED = 0x1008;
const uint16_t NACK_DB_IS_FULL = 0x1009;
const uint16_t NACK_DB_IS_EMPTY = 0x100a;
const uint16_t NACK_TURN_ERR = 0x100b;
const uint16_t NACK_BAD_FINGER = 0x100c;
const uint16_t NACK_ENROLL_FAILED = 0x100d;
const uint16_t NACK_IS_NOT_SUPPORTED = 0x100e;
const uint16_t NACK_DEV_ERR = 0x100f;
const uint16_t NACK_CAPTURE_CANCELED = 0x1010;
const uint16_t NACK_INVALID_PARAM = 0x1011;
const uint16_t NACK_FINGER_IS_NOT_PRESSED = 0x1012;

// UART
static const int RX_BUF_SIZE = 1024;

// Fingerprint
char *cmd_buffer;
char *resp_buffer;
char *data_buffer;

void init_uart(void);
void configure_uart(int baudrate, int rx_pin, int tx_pin);

// Function to print the 12 bytes of a command packet
void print_command();

// Function to print the 12 bytes of a response packet
void print_response();

void write_uart();

// Read UART returns the response code of bytes read
// If no response code was read, returns -1
// Takes a pointer to a char array, and a int of expected bytes to read
int16_t read_uart(uint16_t bytes, uint32_t cursor);

void get_sha256(const char *data, size_t length, uint8_t *output);

// Finger scanner functions
uint16_t Checksum(char *packet, int len);

void ConstructCommandPacket(uint32_t param, uint16_t cmd);

void SendCommand(uint32_t params, char cmd);
void ReadResponse(uint32_t bytes);

void InitFingerprint(void);
void Open(void);
void Close(void);
void LedOn(void);
void LedOff(void);
uint32_t GetEnrolledCount();
void GetImage(void);
void GetRawImage(void);
void GetTemplate(uint8_t id);
bool CaptureFingerFast();
bool CaptureFingerSlow();
bool Identification(uint8_t*);
bool IsFingerPressed();
void Enroll(uint8_t num);
void ChangeBaudrate(uint32_t baudrate);
void DeleteAll();

#endif