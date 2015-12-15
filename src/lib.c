#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <linux/can.h>

#include "lib.h"

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

void fprint_canframe(FILE *stream , struct can_frame *cf, int maxdlen) {
    /* documentation see lib.h */

    char buf[CL_CFSZ]; /* max length */

    sprint_canframe(buf, cf, maxdlen);
    fprintf(stream, "%s", buf);
}

void sprint_canframe(char *buf , struct can_frame *cf, int maxdlen) {
    /* documentation see lib.h */

    int offset;
    int len = (cf->can_dlc > maxdlen) ? maxdlen : cf->can_dlc;

    int isRTR = maxdlen == CAN_MAX_DLEN && (cf->can_id & CAN_RTR_FLAG);

    if (cf->can_id & CAN_ERR_FLAG) {
        buf[0] = isRTR ? 'R' : 'T';
        put_eff_id(buf+1, cf->can_id & (CAN_ERR_MASK|CAN_ERR_FLAG));
        offset = 9;
    } else if (cf->can_id & CAN_EFF_FLAG) {
        buf[0] = isRTR ? 'R' : 'T';
        put_eff_id(buf+1, cf->can_id & CAN_EFF_MASK);
        offset = 9;
    } else {
        buf[0] = isRTR ? 'r' : 't';
        put_sff_id(buf+1, cf->can_id & CAN_SFF_MASK);
        offset = 4;
    }

    buf[offset++] = hex_asc_upper_lo(cf->can_dlc);

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
            put_hex_byte(buf + offset, cf->data[i]);
            offset += 2;
        }
    }

    buf[offset] = 0;
}
