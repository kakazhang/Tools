#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H
#include <pthread.h>

#include "Entity.h"

class TcpClient : public Entity {
    public:
        TcpClient(const char *ip,int port);
        virtual ~TcpClient();
        virtual void init();
        virtual int send(void *buf, size_t len);
        virtual int recv(void *buf, size_t len);
        virtual void exit();

        virtual void onDataAvailable(int fd);
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
