/*
 * server.h
 *
 *  Created on: 30 но€б. 2015 г.
 *      Author: Dmitry
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <mqueue.h>

struct netRxJob {
    int socket;
    mqd_t queue;
};

struct netTxJob {
    int socket;
    mqd_t queue;
};

struct ServerJob {
    int socket;
    mqd_t netRxQueue;
    struct listhead *head;
};

void* serverThread(void *arg);

void *inputConnectionHandler(void *job_ptr);
void *outputConnectionHandler(void *job_ptr);

#endif /* SERVER_H_ */
