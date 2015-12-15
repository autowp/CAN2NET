#include "can.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/queue.h>

#include "common.h"
#include "lib.h"

//int messagesSent = 0;

void* canRxThread(void *arg)
{
    struct canRxJob *job = (struct canRxJob *)arg;

    int canSocket = job->socket;
    struct sockaddr_can *addr = job->addr;


    struct timeval *timeout_current = NULL;



    struct can_frame frame;
    struct iovec iov;
    iov.iov_base = &frame;
    char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))];
    struct msghdr msg;
    msg.msg_name = addr;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &ctrlmsg;

    while (1) {

        fd_set rdfs;
        FD_ZERO(&rdfs);
        FD_SET(canSocket, &rdfs);

        if ((select(canSocket+1, &rdfs, NULL, NULL, timeout_current)) <= 0) {
            perror("select");
            //running = 0;
            continue;
        }

        if (FD_ISSET(canSocket, &rdfs)) {

            iov.iov_len = sizeof(frame);
            msg.msg_namelen = sizeof(struct sockaddr_can);
            msg.msg_controllen = sizeof(ctrlmsg);
            msg.msg_flags = 0;

            int nbytes = recvmsg(canSocket, &msg, 0);
            if (nbytes < 0) {
                perror("read");
                return NULL;
            }

            int maxdlen;
            if ((size_t)nbytes == CAN_MTU)
                maxdlen = CAN_MAX_DLEN;
            else if ((size_t)nbytes == CANFD_MTU)
                maxdlen = CANFD_MAX_DLEN;
            else {
                fputs("read: incomplete CAN frame\n", stderr);
                return NULL;
            }

            if (mq_send(job->queue, (char *)&frame, CAN_MTU, 0) != 0) {
                perror("mq_send");
                return NULL;
            }


            /*struct cmsghdr *cmsg;
            for (cmsg = CMSG_FIRSTHDR(&msg);
                 cmsg && (cmsg->cmsg_level == SOL_SOCKET);
                 cmsg = CMSG_NXTHDR(&msg,cmsg)) {
                if (cmsg->cmsg_type == SO_RXQ_OVFL)
                    dropcnt = *(__u32 *)CMSG_DATA(cmsg);
            }

            if (dropcnt != last_dropcnt) {

                __u32 frames = dropcnt - last_dropcnt;

                printf("DROPCOUNT: dropped %d CAN frame%s on '%s' socket (total drops %d)\n",
                       frames, (frames > 1)?"s":"", cmdlinename, dropcnt);

                last_dropcnt = dropcnt;
            }*/

            fprint_canframe(stdout, &frame, maxdlen);
            printf("\n");
        }

        fflush(stdout);

    }

    return NULL;
}


void* canTxThread(void *arg)
{
    struct canTxJob *job = (struct canTxJob *)arg;

    struct can_frame frame;
    ssize_t bytes_read;

    while ((bytes_read = mq_receive(job->queue, (char *)&frame, CAN_MTU, NULL))) {

        int nbytes = write(job->socket, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            if (errno != ENOBUFS) {
                perror("write");
                return NULL;
            }
            perror("write");
            return NULL;

        } else if (nbytes < sizeof(struct can_frame)) {
            fprintf(stderr, "write: incomplete CAN frame\n");
            return NULL;
        }

    }

    /*unsigned char messageDataIgnition[8] = {0x0E, 0x00, 0x00, 0x0F, 0x01, 0x00, 0x00, 0xA0};
    unsigned char messageDataVIN[8] = {0x32, 0x31, 0x34, 0x39, 0x36, 0x34, 0x36, 0x34};
    CAN.sendMsgBuf(0x2B6, 0, 8, messageDataVIN);
    CAN.sendMsgBuf(0x036, 0, 8, messageDataIgnition);*/

    /*frame.can_id = 0x036;
    frame.can_dlc = 8;
    frame.data[0] = 0x0E;
    frame.data[1] = 0x00;
    frame.data[2] = 0x00;
    frame.data[3] = 0x0F;
    frame.data[4] = 0x01;
    frame.data[5] = 0x0E;
    frame.data[6] = 0x0E;
    frame.data[7] = 0xA0;

    if (sendFrame(s, &frame)) {
        return NULL;
    }

    frame.can_id = 0x2B6;
    frame.can_dlc = 8;
    frame.data[0] = 0x32;
    frame.data[1] = 0x31;
    frame.data[2] = 0x34;
    frame.data[3] = 0x39;
    frame.data[4] = 0x36;
    frame.data[5] = 0x34;
    frame.data[6] = 0x36;
    frame.data[7] = 0x34;

    if (sendFrame(s, &frame)) {
        return NULL;
    }*/

    return NULL;
}
