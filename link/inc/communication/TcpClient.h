#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H
#include <pthread.h>

#include "Entity.h"

class TcpClient : public Entity {
    public:
        TcpClient(const char *ip,int port);
        virtual ~TcpClient();
        virtual void init();
        virtual int send(int fd, void *buf, size_t len);
        virtual int recv(int fd, void *buf, size_t len);
        virtual void exit();

        virtual void handleTimeout();
        virtual void handleException(int fd);

        virtual int getFd() {return mSock;}
    private:
        int createClient();
        int destroyClient();
    private:
        const char *mIP;
        int mPort;
        int mSock;
        pthread_mutex_t mLock;
};
#endif
