#include <sys/queue.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "common.h"
#include "worker.h"
#include "linux/can.h"


void *can2netThread(void *arg)
{
    struct can2netJob *job = (struct can2netJob *)arg;
    struct listhead *head = job->head;

    struct can_frame frame;
    ssize_t bytes_read;

    while ((bytes_read = mq_receive(job->canQueue, (char *)&frame, CAN_MTU, NULL))) {

        puts("can2net: read frame from queue");

        char buffer[NET_MAX_MESSAGE];
        sprint_canframe(buffer, &frame, CAN_MTU);
        int len = strlen(buffer);
        buffer[len] = '\r';
        buffer[len+1] = '\0';

        struct entry *eachEntry;
        LIST_FOREACH(eachEntry, head, entries) {
            if (mq_send(eachEntry->queue, buffer, len+1, 0) != 0) {
                perror("mq_send");
                return NULL;
            }
        }

    }

    return NULL;
}

void *net2canThread(void *arg)
{
    struct net2canJob *job = (struct net2canJob *)arg;

    char buffer[NET_MAX_MESSAGE];
    ssize_t bytesRead;

    while ((bytesRead = mq_receive(job->netQueue, buffer, NET_MAX_MESSAGE, NULL))) {
        struct can_frame frame;

        parseMessage(buffer, bytesRead, &frame);

        if (mq_send(job->canQueue, (char *)&frame, CAN_MTU, 0) != 0) {
            perror("mq_send");
            return NULL;
        }
    }

    return NULL;
}
