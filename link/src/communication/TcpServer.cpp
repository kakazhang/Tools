#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "TcpServer.h"

TcpServer::TcpServer(int port)
    :mSock(-1), mPort(port)
{
    pthread_mutex_init(&mLock, NULL);
}

TcpServer::~TcpServer()
{
    pthread_mutex_destroy(&mLock);
    exit();
}

void TcpServer::init()
{
    createServer();
}

void TcpServer::exit()
{
    destroyServer();
}

int TcpServer::createServer()
{
    struct hostent *host;
    struct sockaddr_in addr;

    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mPort);

    mSock = socket(AF_INET, SOCK_STREAM, 0);
    if (mSock < 0) {
        perror("socket");
        return mSock;
    }

    if (::bind(mSock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(mSock);
        return -1;
    }

    int ret = listen(mSock, 5);
}

int TcpServer::destroyServer()
{
    if (mSock > 0)
        close(mSock);
}

int TcpServer::send(int fd, void *buf, size_t len)
{
    int ret;

    if (fd > 0) {
        ret = ::send(fd, buf, len , 0);
        if (ret < 0) {
            printf("%s send failed:%s\n", __func__, strerror(errno));
            handleException(fd);
        }
    }
    return ret;
}

int TcpServer::recv(int fd, void *buf, size_t len)
{
    int ret = ::recv(fd, buf, len, 0);
    if (ret < 0) {
        printf("%s send failed:%s\n", __func__, strerror(errno));
        handleException(fd);
    }

    return ret;
}

void TcpServer::handleTimeout()
{
    //do nothing
}

void TcpServer::handleException(int fd)
{
    close(fd);
}
