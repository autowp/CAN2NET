#ifndef COMMON_H_
#define COMMON_H_

#include <mqueue.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <linux/can.h>

#define CAN_TX_QUEUE_NAME "/cantxqueue"
#define CAN_RX_QUEUE_NAME "/canrxqueue"
#define SERVER_RX_QUEUE_NAME "/serverrxqueue"
#define SERVER_TX_QUEUE_NAME "/servertxqueue-%d"

#define NET_MAX_MESSAGE 2000

#define CHECK(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            perror(#x); \
            exit(-1); \
        } \
    } while (0) \


LIST_HEAD(listhead, entry);
struct entry {
    mqd_t queue;
    LIST_ENTRY(entry) entries;
};

unsigned char hexCharToByte(char hex);

#endif /* #ifndef COMMON_H_ */
