#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "TcpServer.h"
#include "TcpServerLink.h"

TcpServerLink::TcpServerLink(TcpAttr* tcpAttr)
    :Poll("TcpServer", tcpAttr->attr.timeout)
{
    mServer = new TcpServer(tcpAttr->port);
    mServer->init();
    mSock = mServer->getFd();
    addFd(mSock);
}

TcpServerLink::~TcpServerLink()
{
    delete mServer;
    if (mSock > 0)
        close(mSock);
}

int TcpServerLink::sendData(void* buf, size_t count)
{
    return mServer->send(mClient, buf, count);
}

void TcpServerLink::onDataAvailable(int fd)
{
    char buf[128] = {0};
    if (fd == mSock) {
        mClient = accept(fd, NULL, NULL);
        printf("%s accept:%d\n", __func__, mClient);
        addFd(mClient);
    } else if(fd == mClient) {
        int count = mServer->recv(mClient, buf, sizeof(buf));
        if (count > 0) {
            printf("server read:%s\n", buf);
        } else {
            mServer->handleException(mClient);
            mClient = -1;
        }
    } else if (fd == 0) {
        mServer->handleTimeout();
    }
}

