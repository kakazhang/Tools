#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "Poll.h"

#define EPOLL_HINT 5

Poll::Poll(const char *threadName, int timeoutInMillis)
    :mThreadName(threadName), mTimeout(timeoutInMillis)
{
    init();
}

Poll::~Poll()
{

}

void Poll::init()
{
    eFd = epoll_create(EPOLL_HINT);
    if (eFd < 0) {
        perror("epoll_create");
        ::exit(-1);
    }

    if (pipe(mWakeFds)) {
        perror("pipe");
        ::exit(-1);
    }

    addFd(mWakeFds[0]);

    pthread_t t;
    pthread_create(&t, NULL, entry, this);
}

int Poll::addFd(int fd)
{
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;
    return epoll_ctl(eFd, EPOLL_CTL_ADD, fd, &event);
}

int Poll::removeFd(int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    return epoll_ctl(eFd, EPOLL_CTL_DEL, fd, &event);
}

void Poll::exit()
{
    char quit[1] = {'q'};
    int ret = write(mWakeFds[1], quit, sizeof(quit));
    if (ret < 0) {
        perror("write");
        return;
    }

    for (int i = 0; i < 2; i++)
        close(mWakeFds[i]);
    close(eFd);
}

void* Poll::entry(void *args)
{
    int nfds;
    struct epoll_event events[5];
    Poll* poll = (Poll *)args;

    prctl(PR_SET_NAME, poll->mThreadName);

    while (1) {
        nfds = epoll_wait(poll->eFd, events, 5, poll->mTimeout);
        if (nfds < 0) {
            perror("epoll_wait");
            continue;
        } else if (nfds == 0) {
            printf("timeout\n");
            poll->onDataAvailable(0);
        } else {
            for (int n = 0; n < nfds; ++n) {
                poll->onDataAvailable(events[n].data.fd);
            }
        }
    }

    return NULL;
}
