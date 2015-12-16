/*
 * CanHacker.h
 *
 *  Created on: 17 дек. 2015 г.
 *      Author: Dmitry
 */

#ifndef CANHACKER_H_
#define CANHACKER_H_

#include <linux/can.h>

#define CAN_MIN_DLEN 1
#define HEX_PER_BYTE 2
#define MIN_MESSAGE_DATA_HEX_LENGTH CAN_MIN_DLEN * HEX_PER_BYTE
#define MAX_MESSAGE_DATA_HEX_LENGTH CAN_MAX_DLEN * HEX_PER_BYTE
#define MIN_MESSAGE_LENGTH 5

class CanHacker {
    public:
        CanHacker();
        virtual ~CanHacker();

        int parseTransmit(char *buffer, int length, struct can_frame *frame);
        int createTransmit(struct can_frame *frame, char *buffer, int length);
};

#endif /* CANHACKER_H_ */
