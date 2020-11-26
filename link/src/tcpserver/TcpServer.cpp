#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include<unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<netinet/in.h>

#include "TcpServer.h"

TcpServer::TcpServer()
{
    mSock = -1;
}

TcpServer::~TcpServer()
{
    destroyServer();
}

void TcpServer::init()
{
    createServer();
}

int TcpServer::createServer()
{
    struct hostent *host;
    struct sockaddr_in addr;

    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_addr.s_addr = inet_addr(INADDR_ANY);
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

int TcpServer::send(void *buf, size_t len)
{
    return ::send(mSock, buf, len , 0);
}

int TcpServer::recv(void *buf, size_t len)
{
    return ::recv(mSock, buf, len, 0);
}
