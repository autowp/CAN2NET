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

void *serverThread(void *arg)
{
    struct ServerJob *serverJob = (struct ServerJob *)arg;
    int serverSocket = serverJob->socket;
    struct listhead *head = serverJob->head;

    int serverTxQueueCount = 0;
    struct sockaddr_in clientSocketAddress;
    int clientSocket;
    size_t socketAddrLength = sizeof clientSocketAddress;
    while ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientSocketAddress, (socklen_t*)&socketAddrLength) ))
    {
        if (clientSocket < 0) {
            perror("accept failed");
            return NULL;
        }

        puts("Connection accepted");

        // network tx
        struct mq_attr txAttr;
        txAttr.mq_flags = 0;
        txAttr.mq_maxmsg = 10;
        txAttr.mq_msgsize = NET_MAX_MESSAGE;
        txAttr.mq_curmsgs = 0;

        char serverTxQueueName[80];
        sprintf(serverTxQueueName, SERVER_TX_QUEUE_NAME, serverTxQueueCount++);
        mqd_t netTxQueue = mq_open(serverTxQueueName, O_CREAT | O_RDWR, 0644, &txAttr);
        if (netTxQueue == (mqd_t)-1) {
            perror("mq_open");
            return NULL;
        }

        struct entry *newEntry = (struct entry *)malloc(sizeof(struct entry));
        newEntry->queue = netTxQueue;
        LIST_INSERT_HEAD(head, newEntry, entries);

        struct netTxJob *txJob = (struct netTxJob *)malloc(sizeof(struct netTxJob));
        txJob->socket = clientSocket;
        txJob->queue = netTxQueue;

        pthread_t txThreadId;
        if (pthread_create(&txThreadId, NULL, outputConnectionHandler, (void*)txJob) < 0) {
            perror("could not create thread");
            return NULL;
        }

        // network rx
        struct netRxJob *rxJob = (struct netRxJob *)malloc(sizeof(struct netRxJob));
        rxJob->socket = clientSocket;
        rxJob->queue = serverJob->netRxQueue;

        pthread_t rxThreadId;
        if (pthread_create(&rxThreadId, NULL, inputConnectionHandler, (void*)rxJob) < 0) {
            perror("could not create thread");
            return NULL;
        }

        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }

    return NULL;
}

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
