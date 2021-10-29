#ifndef CAN_H_
#define CAN_H_

#include <mqueue.h>

struct canRxJob {
    int socket;
    mqd_t queue;
    struct sockaddr_can *addr;
};

struct canTxJob {
    int socket;
    mqd_t queue;
    struct sockaddr_can *addr;
};

void* canRxThread(void *arg);
void* canTxThread(void *arg);

#endif /* CAN_H_ */
