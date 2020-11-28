#ifndef ENTITY_H
#define ENTITY_H

typedef int (*onEvent)(void* buf, size_t count);

class Entity {
    public:
        virtual void init() = 0;
        virtual int send(int fd, void* buf, size_t len) = 0;
        virtual int recv(int fd, void *buf, size_t len) = 0;
        virtual void exit() = 0;

        virtual int getFd() = 0;

        virtual void handleTimeout() = 0;
        virtual void handleException(int fd) = 0;
};

#endif
