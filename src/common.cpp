#include "common.h"


#include <stdio.h>
#include <string.h>

#define ID_HEX_LENGTH 3
#define SEPARATOR_LENGTH 1
#define CAN_MIN_DLEN 1
#define HEX_PER_BYTE 2
#define MIN_MESSAGE_DATA_HEX_LENGTH CAN_MIN_DLEN * HEX_PER_BYTE
#define MAX_MESSAGE_DATA_HEX_LENGTH CAN_MAX_DLEN * HEX_PER_BYTE
#define MIN_MESSAGE_LENGTH ID_HEX_LENGTH + SEPARATOR_LENGTH + CAN_MIN_DLEN * HEX_PER_BYTE
#define MAX_MESSAGE_LENGTH ID_HEX_LENGTH + SEPARATOR_LENGTH + CAN_MAX_DLEN * HEX_PER_BYTE

unsigned char hexCharToByte(char hex)
{
    unsigned char result = 0;
    if (hex >= 0x30 && hex <= 0x39) {
        result = hex - 0x30;
    } else if (hex >= 0x41 && hex <= 0x46) {
        result = hex - 0x41 + 0x0A;
    } else if (hex >= 0x61 && hex <= 0x66) {
        result = hex - 0x61 + 0x0A;
    } else {
        fputs("# Unexpected hex char", stderr);
    }
    return result;
}
