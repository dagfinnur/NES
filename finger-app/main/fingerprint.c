#include "fingerprint.h"

void SendCommand(){

};

__uint16_t Checksum(char* packet, int packet_len) {
    __uint16_t sum = 0;

    for (int i = 0; i < packet_len; i++) {
        sum += packet[i];
    }
    printf("Checksum %d\n", sum);
    fflush(stdout);
    return sum;
};

void ConstructCommandPacket(__uint32_t param, __uint16_t cmd, char* cmd_arr) {
    cmd_arr[0] = COMMAND_START_CODE_1;
    cmd_arr[1] = COMMAND_START_CODE_2;
    
    cmd_arr[2] = DEVICE_ID_1;
    cmd_arr[3] = DEVICE_ID_2;
    
    cmd_arr[4] = param & 0xff;
    cmd_arr[5] = (param >> 8) & 0xff;
    cmd_arr[6] = (param >> 16) & 0xff;
    cmd_arr[7] = (param >> 24) & 0xff;
    
    cmd_arr[8] = cmd & 0xff;
    cmd_arr[9] = (cmd >> 8) & 0xff;

    __uint16_t checksum = Checksum(cmd_arr, 10);
    

    cmd_arr[10] = checksum & 0xff;
    cmd_arr[11] = (checksum >> 8) & 0xff;   

    return;
};

void Open(){
    char *cmd_array = (char *)malloc(12 * sizeof(char));
    char params = 0x00;
    char cmd = CMD_OPEN;
    
    ConstructCommandPacket(params, cmd, cmd_array);
    
    // Send command
    
    // Get response

    // Free arrays
    free(cmd_array);
}