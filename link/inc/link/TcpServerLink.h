#ifndef TCPSERVERLINK_H
#define TCPSERVERLINK_H

#include "common.h"
#include "Poll.h"
#include "TcpServer.h"

class TcpServerLink : public Poll {
    public:
        TcpServerLink(TcpAttr* tcpAttr);
        virtual ~TcpServerLink();

        virtual int sendData(void *buf, size_t count);
        virtual void onDataAvailable(int fd);
    private:
        TcpServer* mServer;
        int mSock;
        int mClient;
};

#endif
