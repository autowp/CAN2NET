/*
 * server.c
 *
 *  Created on: 30 ����. 2015 �.
 *      Author: Dmitry
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#include "server.h"
#include "common.h"
#include "lib.h"

void *inputConnectionHandler(void *job_ptr)
{
    //Get the socket descriptor
    struct netRxJob *job = (struct netRxJob *)job_ptr;

    char clientMessage[NET_MAX_MESSAGE];

    //Receive a message from client
    ssize_t readSize;
    while ( (readSize = recv(job->socket, clientMessage , NET_MAX_MESSAGE, 0)) > 0 ) {
        if (mq_send(job->queue, clientMessage, readSize, 0) != 0) {
            perror("mq_send");
            return NULL;
        }
    }



    /*if (read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if(read_size == -1) {
        perror("recv failed");
    }*/

    if (close(job->socket)) {
        perror("close");
        return NULL;
    }

    free(job_ptr);

    return NULL;
}

void *outputConnectionHandler(void *job_ptr)
{
    //Get the socket descriptor
    struct netTxJob *job = (struct netTxJob *)job_ptr;

    //char client_message[2000];

    //ssize_t bytes_read;
    char clientMessage[NET_MAX_MESSAGE];

    ssize_t bytes_read;

    /* receive the message */
    while ((bytes_read = mq_receive(job->queue, clientMessage, NET_MAX_MESSAGE, NULL))) {
        if (bytes_read == -1) {
            perror("mq_receive");
            return NULL;
        }
        puts("netTx: read message from queue");
        //char buf[CL_CFSZ];
        //sprint_canframe(buf, &frame, 1, CAN_MAX_DLEN);


        write(job->socket, clientMessage, bytes_read);
    }

    /*if (read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if(read_size == -1) {
        perror("recv failed");
    }*/

    if (mq_close(job->queue)) {
        perror("mq_close");
        return NULL;
    }

    if (close(job->socket)) {
        perror("close");
        return NULL;
    }

    free(job_ptr);

    return NULL;
}
