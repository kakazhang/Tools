#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include <pthread.h>

#include "Entity.h"

class TcpServer : public Entity {
    public:
        TcpServer(int port);
        virtual ~TcpServer();
        virtual void init();
        virtual int send(int fd, void *buf, size_t len);
        virtual int recv(int fd, void *buf, size_t len);
        virtual void exit();

        virtual void handleTimeout();
        virtual void handleException(int fd);

        virtual int getFd() {return mSock;}
    private:
        int createServer();
        int destroyServer();
    private:
        int mSock;
        int mPort;
        pthread_mutex_t mLock;
};
#endif
