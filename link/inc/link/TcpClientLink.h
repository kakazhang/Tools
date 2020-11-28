#ifndef TCPCLIENTLINK_H
#define TCPCLIENTLINK_H

#include "common.h"
#include "Poll.h"
#include "TcpClient.h"

class TcpClientLink : public Poll {
    public:
        TcpClientLink(TcpAttr* tcpAttr);
        virtual ~TcpClientLink();

        virtual int sendData(void *buf, size_t count);
        virtual void onDataAvailable(int fd);
    private:
        TcpClient* mClient;
        int mSock;
};

#endif
