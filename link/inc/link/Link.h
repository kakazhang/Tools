#ifndef LINK_H
#define LINK_H

class Link {
    public:
        virtual void init() = 0;
        virtual int send(void* buf, size_t len) = 0;
        virtual int recv(void *buf, size_t len) = 0;
};

#endif
