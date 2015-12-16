#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <mqueue.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/queue.h>

#include "can.h"
#include "common.h"
#include "server.h"
#include "worker.h"
#include "CanHacker.h"

const char *cmdlinename = "can0";
const int serverPort = 20100;
static volatile int running = 1;

struct listhead head;

CanHacker canHacker;

void sigterm(int signo)
{
    running = 0;
}

int initServer()
{
    int serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in adr_srvr;
    memset(&adr_srvr, 0, sizeof adr_srvr);
    adr_srvr.sin_family = AF_INET;
    adr_srvr.sin_port = htons(serverPort);
    adr_srvr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&adr_srvr, sizeof adr_srvr) == -1) {
        perror("bind(2)");
        return -1;
    }

    if (listen(serverSocket, 10) == -1) {
        perror("listen(2)");
        return -1;
    }

    return serverSocket;
}

int initCan(struct sockaddr_can *addr)
{
    int canSocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (canSocket < 0) {
        perror("socket");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
    strncpy(ifr.ifr_name, cmdlinename, strlen(cmdlinename));

    if (ioctl(canSocket, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        return -1;
    }

    if (!ifr.ifr_ifindex) {
        perror("invalid interface");
        return 1;
    }

    // try to switch the socket into CAN FD mode
    /*const int canfd_on = 1;
    if (setsockopt(canSocket, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on)) < 0) {
        perror("setsockopt CAN_RAW_FD_FRAMES");
        return -1;
    }*/

    /*const int dropmonitor_on = 1;
    if (setsockopt(canSocket, SOL_SOCKET, SO_RXQ_OVFL,
               &dropmonitor_on, sizeof(dropmonitor_on)) < 0) {
        perror("setsockopt SO_RXQ_OVFL not supported by your Linux Kernel");
        return -1;
    }*/


    addr->can_family = AF_CAN;
    addr->can_ifindex = ifr.ifr_ifindex;

    if (bind(canSocket, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("bind");
        return -1;
    }

    return canSocket;
}

int main(int argc, char **argv)
{
    signal(SIGTERM, sigterm);
    signal(SIGHUP, sigterm);
    signal(SIGINT, sigterm);

    /* initialize the queue attributes */


    /* create the message queue */
    int err;

    /*pthread_t serverThreadId;

    err = pthread_create(&serverThreadId, NULL, &serverThread, queues);
    if (err != 0) {
        printf("Can't create server thread :[%s]", strerror(err));
    } else {
        printf("Server thread created successfully\n");
    }*/

    // init queues
    LIST_INIT(&head);

    struct mq_attr canTxAttr;
    canTxAttr.mq_flags = 0;
    canTxAttr.mq_maxmsg = 10;
    canTxAttr.mq_msgsize = CAN_MTU;
    canTxAttr.mq_curmsgs = 0;

    mqd_t canTxQueue = mq_open(CAN_TX_QUEUE_NAME, O_CREAT | O_RDWR, 0666, &canTxAttr);
    if (canTxQueue == (mqd_t)-1) {
        fprintf(stderr, "error: %d\n", errno);
        perror("mq_open");
        return -1;
    }

    struct mq_attr canRxAttr;
    canRxAttr.mq_flags = 0;
    canRxAttr.mq_maxmsg = 10;
    canRxAttr.mq_msgsize = CAN_MTU;
    canRxAttr.mq_curmsgs = 0;

    mqd_t canRxQueue = mq_open(CAN_RX_QUEUE_NAME, O_CREAT | O_RDWR, 0644, &canRxAttr);
    if (canRxQueue == (mqd_t)-1) {
        perror("mq_open");
        return -1;
    }

    // init sockets
    struct sockaddr_can addr;
    int canSocket = initCan(&addr);
    if (canSocket == -1) {
        return -1;
    }

    int serverSocket = initServer();
    if (serverSocket == -1) {
        return -1;
    }

    // init threads
    struct canTxJob txCanJob;
    txCanJob.addr = &addr;
    txCanJob.queue = canTxQueue;
    txCanJob.socket = canSocket;

    pthread_t canTxThreadId;

    err = pthread_create(&canTxThreadId, NULL, &canTxThread, &txCanJob);
    if (err != 0) {
        printf("Can't create CAN TX thread :[%s]", strerror(err));
    } else {
        printf("CAN TX thread created successfully\n");
    }

    struct canRxJob rxCanJob;
    rxCanJob.addr = &addr;
    rxCanJob.queue = canRxQueue;
    //rxCanJob.head = &head;
    rxCanJob.socket = canSocket;

    pthread_t canRxThreadId;

    err = pthread_create(&canRxThreadId, NULL, &canRxThread, &rxCanJob);
    if (err != 0) {
        printf("Can't create CAN TX thread :[%s]", strerror(err));
    } else {
        printf("CAN TX thread created successfully\n");
    }

    // network rx queue
    struct mq_attr rxAttr;
    rxAttr.mq_flags = 0;
    rxAttr.mq_maxmsg = 10;
    rxAttr.mq_msgsize = NET_MAX_MESSAGE;
    rxAttr.mq_curmsgs = 0;

    mqd_t netRxQueue = mq_open(SERVER_RX_QUEUE_NAME, O_CREAT | O_RDWR, 0644, &rxAttr);
    if (netRxQueue == (mqd_t)-1) {
        perror("mq_open");
        return -1;
    }


    // workers
    struct net2canJob net2canJob;
    net2canJob.netQueue = netRxQueue;
    net2canJob.canQueue = canTxQueue;
    net2canJob.canHacker = &canHacker;

    pthread_t net2canThreadID;
    err = pthread_create(&net2canThreadID, NULL, &net2canThread, &net2canJob);
    if (err != 0) {
        printf("Can't create net2can thread :[%s]", strerror(err));
    } else {
        printf("net2can thread created successfully\n");
    }

    struct can2netJob can2netJob;
    can2netJob.canQueue = canRxQueue;
    can2netJob.head = &head;
    can2netJob.canHacker = &canHacker;

    pthread_t can2netThreadID;
    err = pthread_create(&can2netThreadID, NULL, &can2netThread, &can2netJob);
    if (err != 0) {
        printf("Can't create can2net thread :[%s]", strerror(err));
    } else {
        printf("can2net thread created successfully\n");
    }

    struct ServerJob serverJob;
    serverJob.socket = serverSocket;
    serverJob.netRxQueue = netRxQueue;

    pthread_t serverThreadID;
    err = pthread_create(&serverThreadID, NULL, &serverThread, &serverJob);
    if (err != 0) {
        printf("Can't create server thread :[%s]", strerror(err));
    } else {
        printf("Server thread created successfully\n");
    }

    while (running) {
        sleep(1);
    }

    if (pthread_cancel(serverThreadID)) {
        perror("pthread_cancel");
        return -1;
    }

    close(canSocket);
    close(serverSocket);

    if (mq_close(netRxQueue)) {
        perror("mq_close");
        return -1;
    }

    if (mq_close(canTxQueue)) {
        perror("mq_close");
        return -1;
    }

    if (mq_close(canRxQueue)) {
        perror("mq_close");
        return -1;
    }

    struct entry *eachEntry;
    LIST_FOREACH(eachEntry, &head, entries) {
        if (mq_close(eachEntry->queue)) {
            perror("mq_close");
            return -1;
        }
    }

    return 0;
}
