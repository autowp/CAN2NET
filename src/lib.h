#include <linux/can.h>
#include <stdio.h>

/* buffer sizes for CAN frame string representations */

#define CL_ID (sizeof("12345678##1"))
#define CL_DATA sizeof(".AA")
#define CL_BINDATA sizeof(".10101010")

 /* CAN FD ASCII hex short representation with DATA_SEPERATORs */
#define CL_CFSZ (2*CL_ID + 64*CL_DATA)

/* CAN DLC to real data length conversion helpers especially for CAN FD */

void fprint_canframe(FILE *stream , struct can_frame *cf, int maxdlen);
void sprint_canframe(char *buf , struct can_frame *cf, int maxdlen);
/*
 * Creates a CAN frame hexadecimal output in compact format.
 * The CAN data[] is separated by '.' when sep != 0.
 *
 * The type of the CAN frame (CAN 2.0 / CAN FD) is specified by maxdlen:
 * maxdlen = 8 -> CAN2.0 frame
 * maxdlen = 64 -> CAN FD frame
 *
 * 12345678#112233 -> extended CAN-Id = 0x12345678, len = 3, data, sep = 0
 * 12345678#R -> extended CAN-Id = 0x12345678, RTR, len = 0
 * 12345678#R5 -> extended CAN-Id = 0x12345678, RTR, len = 5
 * 123#11.22.33.44.55.66.77.88 -> standard CAN-Id = 0x123, dlc = 8, sep = 1
 * 32345678#112233 -> error frame with CAN_ERR_FLAG (0x2000000) set
 * 123##0112233 -> CAN FD frame standard CAN-Id = 0x123, flags = 0, len = 3
 * 123##2112233 -> CAN FD frame, flags = CANFD_ESI, len = 3
 *
 * Examples:
 *
 * fprint_canframe(stdout, &frame, "\n", 0); // with eol to STDOUT
 * fprint_canframe(stderr, &frame, NULL, 0); // no eol to STDERR
 *
 */

