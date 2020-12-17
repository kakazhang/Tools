#ifndef UARTSESSION_H
#define UARTSESSION_H
#include <pthread.h>

#include "Entity.h"
#include "common.h"

class UartSession : public Entity {
    public:
        UartSession(UartAttr* attr);
        virtual ~UartSession();
        virtual void init();
        virtual int send(int fd, void *buf, size_t len);
        virtual int recv(int fd, void *buf, size_t len);
        virtual void exit();

        virtual void handleTimeout();
        virtual void handleException(int fd);

        virtual int getFd() {return uFd;}
    private:
        int createUart();
        int closeUart();
    private:
        UartAttr *mUartAttr;
        int uFd;
        pthread_mutex_t mLock;
};
#endif
