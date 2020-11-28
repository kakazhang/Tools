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
#include<netinet/in.h>

#include "TcpClient.h"

TcpClient::TcpClient(const char *ip,int port)
    :mIP(ip),mPort(port),mSock(-1)
{
    pthread_mutex_init(&mLock, NULL);
}

TcpClient::~TcpClient()
{
    pthread_mutex_destroy(&mLock);
    exit();
}

void TcpClient::init()
{
    createClient();
}

void TcpClient::exit()
{
    destroyClient();
}

int TcpClient::createClient()
{
    struct hostent *host;
    struct sockaddr_in saddr;

    bzero(&saddr, sizeof(struct sockaddr_in));
    saddr.sin_addr.s_addr = inet_addr(mIP);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(mPort);

    mSock = socket(AF_INET, SOCK_STREAM, 0);
    if (mSock < 0) {
        perror("socket");
        return mSock;
    }

    int keepAlive = 1;
    setsockopt(mSock, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));

    fcntl(mSock, F_SETFL, fcntl(mSock, F_GETFL, 0)|O_NONBLOCK);
    int connected = connect(mSock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    return connected;
}

int TcpClient::destroyClient()
{
    if (mSock > 0) {
        close(mSock);
        mSock = -1;
    }
}

int TcpClient::send(int fd, void *buf, size_t len)
{
    int ret = -1;
    if (fd > 0) {
        ret =  ::send(fd, buf, len , 0);
        if (ret < 0) {
            printf("send failed:%s\n", strerror(errno));
            handleException(fd);
        }
    }

    return ret;
}

int TcpClient::recv(int fd, void *buf, size_t len)
{
    int ret = ::recv(fd, buf, len, 0);
    if (ret < 0) {
        printf("send failed:%s\n", strerror(errno));
        handleException(fd);
    }

    return ret;
}

void TcpClient::handleTimeout()
{
    if (mSock < 0)
        createClient();
}

void TcpClient::handleException(int fd)
{
    if (fd == mSock)
        destroyClient();
}
