#ifndef POLL_H
#define POLL_H

#include <unistd.h>

class Poll {
    public:
        Poll(const char *threadName, int timeoutInMillis);
        virtual ~Poll();
        void init();
        int addFd(int fd);
        int removeFd(int fd);
        void exit();

        virtual int sendData(void *buf, size_t count) = 0;
        virtual void onDataAvailable(int fd) = 0;
    protected:
        static void* entry(void *args);
    private:
        int eFd;
        int mTimeout;
        const char *mThreadName;
        int mWakeFds[2];
};
#endif
