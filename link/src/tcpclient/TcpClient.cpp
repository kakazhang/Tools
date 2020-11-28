#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include<unistd.h>
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

    fcntl(mSock, F_SETFL, fcntl(mSock, F_GETFL, 0)|O_NONBLOCK);
    int connected = connect(mSock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    return connected;
}

int TcpClient::destroyClient()
{
    if (mSock > 0)
        close(mSock);
}

int TcpClient::send(void *buf, size_t len)
{
    return ::send(mSock, buf, len , 0);
}

int TcpClient::recv(void *buf, size_t len)
{
    return ::recv(mSock, buf, len, 0);
}

void TcpClient::onDataAvailable(int fd)
{
}
