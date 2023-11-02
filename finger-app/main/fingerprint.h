#ifndef FINGER_PRINT
#define FINGER_PRINT

#include <stdio.h>
#include <string.h>

const char COMMAND_START_CODE_1 = 0x55;
const char COMMAND_START_CODE_2 = 0xAA;
const char DEVICE_ID_1 = 0x01;
const char DEVICE_ID_2 = 0x00;

// Commands
const char CMD_OPEN = 0x01;
const char CMD_CLOSE = 0x02;
const char CMD_USB_INTERNAL_CHECK = 0x03;
const char CMD_SET_IAP_MODE = 0x04;
const char CMD_CMOS_LED = 0x12;

const char CMD_ACK = 0x30;
const char CMD_NACK = 0x31;


// Error codes


// Functions
void SendCommand();


__uint16_t Checksum(char *packet, int len);

void ConstructCommandPacket(__uint32_t param, __uint16_t cmd, char *cmd_arr);
void Open();

#endif