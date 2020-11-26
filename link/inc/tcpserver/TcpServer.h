#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "Link.h"

class TcpServer : public Link {
    public:
        TcpServer();
        virtual ~TcpServer();
        virtual void init();
        virtual int send(void *buf, size_t len);
        virtual int recv(void *buf, size_t len);
    private:
        int createServer();
        int destroyServer();
    private:
        int mSock;
        int mClient;
};
#endif
