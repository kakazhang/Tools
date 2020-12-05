#include <stdio.h>
#include <stdlib.h>

#include "TcpClientLink.h"

TcpClientLink::TcpClientLink(TcpAttr* tcpAttr)
    :Poll("TcpClient", tcpAttr->attr.timeout)
{
    mClient = new TcpClient(tcpAttr->ip, tcpAttr->port);
    mClient->init();
    mSock = mClient->getFd();
    addFd(mSock);
}

TcpClientLink::~TcpClientLink()
{
    delete mClient;
}

int TcpClientLink::sendData(void *buf, size_t count)
{
    return mClient->send(mSock, buf, count);
}

void TcpClientLink::onDataAvailable(int fd)
{
    char buf[128] = {0};

    if (fd == -1) {
        mClient->handleTimeout();
        return;
    }

    int count = mClient->recv(mSock, buf, sizeof(buf));
    if (count > 0)
        printf("Client read:%s\n", buf);
}


