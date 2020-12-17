#include <stdio.h>

#include "common.h"
#include "Poll.h"
#include "UartSession.h"
#include "UartLink.h"

UartLink::UartLink(UartAttr* uartAttr)
    :Poll("UartLink", uartAttr->attr.timeout)
{
    mUart = new UartSession(uartAttr);
    mUart->init();
    addFd(mUart->getFd());
}

UartLink::~UartLink()
{
    delete mUart;
}

int UartLink::sendData(void *buf, size_t count)
{
    return mUart->send(mUart->getFd(), buf, count);
}

void UartLink::onDataAvailable(int fd)
{
    char buf[1024] = {0};

    if (fd == -1) {
        mUart->handleTimeout();
        return;
    }

    int count = mUart->recv(mUart->getFd(), buf, sizeof(buf));
    if (count > 0) {
        for (int i = 0; i < count; i++)
            printf("%2x ", buf[i]);

        printf("\n");
    }
}
