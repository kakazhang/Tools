#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "Link.h"

class TcpClient : public Link {
    public:
        TcpClient(const char *ip,int port);
        virtual ~TcpClient();
        virtual void init();
        virtual int send(void *buf, size_t len);
        virtual int recv(void *buf, size_t len);
    private:
        int createClient();
        int destroyClient();
    private:
        const char *mIP;
        int mPort;
        int mSock;
};
#endif
