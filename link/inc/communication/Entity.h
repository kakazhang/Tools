#ifndef ENTITY_H
#define ENTITY_H

typedef int (*onEvent)(void* buf, size_t count);

class Entity {
    public:
        virtual void init() = 0;
        virtual int send(void* buf, size_t len) = 0;
        virtual int recv(void *buf, size_t len) = 0;
        virtual void exit() = 0;

        virtual void onDataAvailable(int fd) = 0;

        virtual void setCallback(onEvent callback) { mCallback = callback;}
    private:
        onEvent mCallback;
};

#endif
