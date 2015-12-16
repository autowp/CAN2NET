/*
 * CanHacker.cpp
 *
 *  Created on: 17 дек. 2015 г.
 *      Author: Dmitry
 */
#include <stdio.h>

#include "CanHacker.h"
#include "common.h"

const char hex_asc_upper[] = "0123456789ABCDEF";

#define hex_asc_upper_lo(x)    hex_asc_upper[((x) & 0x0F)]
#define hex_asc_upper_hi(x)    hex_asc_upper[((x) & 0xF0) >> 4]

static inline void put_hex_byte(char *buf, __u8 byte)
{
    buf[0] = hex_asc_upper_hi(byte);
    buf[1] = hex_asc_upper_lo(byte);
}

static inline void _put_id(char *buf, int end_offset, canid_t id)
{
    /* build 3 (SFF) or 8 (EFF) digit CAN identifier */
    while (end_offset >= 0) {
        buf[end_offset--] = hex_asc_upper[id & 0xF];
        id >>= 4;
    }
}

#define put_sff_id(buf, id) _put_id(buf, 2, id)
#define put_eff_id(buf, id) _put_id(buf, 7, id)

CanHacker::CanHacker ()
{


}

CanHacker::~CanHacker ()
{

}

int CanHacker::parseTransmit(char *buffer, int length, struct can_frame *frame) {
    if (length < MIN_MESSAGE_LENGTH) {
        fputs("Broken message. Length < 6. Skip", stderr);
        return -1;
    }

    int isExended = 0;
    int isRTR = 0;

    switch (buffer[0]) {
        case 't':
            break;
        case 'T':
            isExended = 1;
            break;
        case 'r':
            isRTR = 1;
            break;
        case 'R':
            isExended = 1;
            isRTR = 1;
            break;
        default:
            fputs("Unexpected command", stderr);
            return -1;

    }

    int offset = 1;

    canid_t id = 0;
    int idChars = isExended ? 8 : 3;
    for (int i=0; i<idChars; i++) {
        id += hexCharToByte(buffer[offset++]);
    }
    if (isRTR) {
        id |= CAN_RTR_FLAG;
    }
    if (isExended) {
        id |= CAN_EFF_FLAG;
    }
    frame->can_id = id;

    __u8 dlc = hexCharToByte(buffer[offset++]);
    if (dlc > 8) {
        fputs("Unexpected dlc", stderr);
        return -1;
    }
    frame->can_dlc = dlc;

    if (!isRTR) {
        for (int i=0; i<dlc; i++) {
            char loHex = buffer[offset++];
            char hiHex = buffer[offset++];
            frame->data[i] = hexCharToByte(loHex) + (hexCharToByte(hiHex) << 4);
        }
    }

    return 0;
}

int CanHacker::createTransmit(struct can_frame *frame, char *buffer, int length) {
    int offset;
    int len = frame->can_dlc;

    int isRTR = frame->can_id & CAN_RTR_FLAG;

    if (frame->can_id & CAN_ERR_FLAG) {
        buffer[0] = isRTR ? 'R' : 'T';
        put_eff_id(buffer+1, frame->can_id & (CAN_ERR_MASK|CAN_ERR_FLAG));
        offset = 9;
    } else if (frame->can_id & CAN_EFF_FLAG) {
        buffer[0] = isRTR ? 'R' : 'T';
        put_eff_id(buffer+1, frame->can_id & CAN_EFF_MASK);
        offset = 9;
    } else {
        buffer[0] = isRTR ? 'r' : 't';
        put_sff_id(buffer+1, frame->can_id & CAN_SFF_MASK);
        offset = 4;
    }

    buffer[offset++] = hex_asc_upper_lo(frame->can_dlc);

    /*if (maxdlen == CANFD_MAX_DLEN) {
        // add CAN FD specific escape char and flags
        buf[offset++] = '#';
        buf[offset++] = hex_asc_upper[cf->flags & 0xF];
        if (sep && len)
            buf[offset++] = '.';
    }*/

    if (!isRTR) {
        int i;
        for (i = 0; i < len; i++) {
            put_hex_byte(buffer + offset, frame->data[i]);
            offset += 2;
        }
    }

    buffer[offset] = 0;

    return 0;
}
