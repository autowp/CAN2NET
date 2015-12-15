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

void parseMessage(char *readBuffer, int length, struct can_frame *frame)
{
    if (length < MIN_MESSAGE_LENGTH) {
        fputs("# Broken message. Length < 6. Skip", stderr);
        return;
    }

    unsigned char id1 = hexCharToByte(readBuffer[0]);
    unsigned char id2 = hexCharToByte(readBuffer[1]);
    unsigned char id3 = hexCharToByte(readBuffer[2]);

    frame->can_id = (id1 << 8) + (id2 << 4) + id3;

    if (readBuffer[3] != 0x20) {
        char buffer[100];
        memset(buffer, 0, sizeof(buffer));
        char msg[] = "# Broken message. Space expected (%X given). Skip";
        sprintf(buffer, msg, readBuffer[3]);
        fputs(buffer, stderr);

        return;
    }

    int hexLen = length - ID_HEX_LENGTH - SEPARATOR_LENGTH;

    if (hexLen < MIN_MESSAGE_DATA_HEX_LENGTH || hexLen > MAX_MESSAGE_DATA_HEX_LENGTH) {
        fputs("# Broken message. 2 - 16 length expected. Skip", stderr);
        return;
    }

    frame->can_dlc = hexLen >> 1;

    int offset = ID_HEX_LENGTH + SEPARATOR_LENGTH;

    int i;
    for (i=0; i<frame->can_dlc; i++) {
        char loHex = readBuffer[offset + i * HEX_PER_BYTE + 1];
        char hiHex = readBuffer[offset + i * HEX_PER_BYTE];
        frame->data[i] = hexCharToByte(loHex) + (hexCharToByte(hiHex) << 4);
    }
}
