#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include <pthread.h>

#include "Entity.h"

class TcpServer : public Entity {
    public:
        TcpServer(int port);
        virtual ~TcpServer();
        virtual void init();
        virtual int send(void *buf, size_t len);
        virtual int recv(void *buf, size_t len);
        virtual void exit();

        virtual void onDataAvailable(int fd);
    private:
        int createServer();
        int destroyServer();
    private:
        int mSock;
        int mClient;
        int mPort;
        pthread_mutex_t mLock;
};
#endif
