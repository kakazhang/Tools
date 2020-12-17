#ifndef UARTLINK_H
#define UARTLINK_H

#include "common.h"
#include "Poll.h"
#include "UartSession.h"

class UartLink : public Poll {
    public:
        UartLink(UartAttr* attr);
        virtual ~UartLink();

        virtual int sendData(void *buf, size_t count);
        virtual void onDataAvailable(int fd);
    private:
        UartSession* mUart;
        int uFd;
};

#endif
