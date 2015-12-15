#ifndef WORKER_H_
#define WORKER_H_

#include <mqueue.h>

struct can2netJob {
    mqd_t canQueue;
    struct listhead *head;
};

struct net2canJob {
    mqd_t canQueue;
    mqd_t netQueue;
};

void *can2netThread(void *arg);

void *net2canThread(void *arg);

#endif /* #ifndef WORKER_H_ */
