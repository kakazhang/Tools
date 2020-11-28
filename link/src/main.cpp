#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"
#include "Poll.h"
#include "TcpClientLink.h"
#include "TcpServerLink.h"

#define FORMAT "Send from %s"

#define TYPE_SERVER 0
#define TYPE_CLIENT 1

int type = TYPE_SERVER;

void* work(void *args)
{
    Poll* link = (Poll *)args;
    char buf[128] = {0};

    int count = snprintf(buf, sizeof(buf), FORMAT, type == TYPE_SERVER?"tcpserver":"tcpclient");
    while (1) {
        sleep(1);

        link->sendData(buf, count);
    }
}

void usage()
{
    printf("Link TYPE_SERVER/TYPE_CLIENT $ip, $port\n");
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 4)
        usage();

    type = atoi(argv[1]);

    TcpAttr tcpAttr;
    tcpAttr.attr.type = type;
    tcpAttr.attr.timeout = 1000;

    tcpAttr.ip = argv[2];
    tcpAttr.port = atoi(argv[3]);

    Poll *link = NULL;
    if (type == TYPE_CLIENT) {
        link = new TcpClientLink(&tcpAttr);
    } else {
        link = new TcpServerLink(&tcpAttr);
    }

    pthread_t t;
    pthread_create(&t, NULL, work, link);

    while (1)
        sleep(1);

    return 0;
}
