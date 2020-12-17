#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "UartSession.h"

UartSession::UartSession(UartAttr* attr)
    : mUartAttr(attr),uFd(-1)
{
    pthread_mutexattr_t rattr;
    pthread_mutexattr_init(&rattr);
    pthread_mutexattr_settype(&rattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mLock, &rattr);
    pthread_mutexattr_destroy(&rattr);
}

UartSession::~UartSession()
{
    exit();
    pthread_mutex_destroy(&mLock);
}

void UartSession::init()
{
    createUart();
}

void UartSession::exit()
{
    closeUart();
}

int UartSession::send(int fd, void *buf, size_t len)
{
    int count = -1;
    pthread_mutex_lock(&mLock);
    if (fd == uFd)
        count = write(uFd, buf, len);
    pthread_mutex_unlock(&mLock);

    return count;
}

int UartSession::recv(int fd, void *buf, size_t len)
{
    int count = -1;
    pthread_mutex_lock(&mLock);
    if (fd == uFd)
        count = read(uFd, buf, len);
    pthread_mutex_unlock(&mLock);

    return count;

}

void UartSession::handleTimeout()
{
    pthread_mutex_lock(&mLock);
    if (uFd < 0)
        createUart();
    pthread_mutex_unlock(&mLock);
}

void UartSession::handleException(int fd)
{
    closeUart();
}

int UartSession::createUart()
{
    struct termios options;
    int bauds[]     = {B115200, B19200, B9600, B4800, B2400, B1200, B300};
    int baudRates[] = {115200, 19200, 9600, 4800, 2400, 1200, 300};
    int nameLen     = sizeof(baudRates)/sizeof(baudRates[0]);

    pthread_mutex_lock(&mLock);
    closeUart();
    uFd = open(mUartAttr->dev_path, O_RDWR);
    if (uFd < 0) {
        perror("open");
        return -1;
    }

    if (tcgetattr(uFd, &options) == -1) {
        perror("tcgetattr");
        return -1;
    }

    cfmakeraw(&options);
    for (int i = 0; i < nameLen; i++) {
        if (baudRates[i] == mUartAttr->baud) {
             cfsetispeed(&options, bauds[i]);
             cfsetospeed(&options, bauds[i]);
        }
        options.c_cflag |= (CLOCAL | CREAD);
    }

    switch(mUartAttr->flow_ctrl) {
        case 0:
            options.c_cflag &= ~CRTSCTS;
            break;
        case 1:
            options.c_cflag |= CRTSCTS;
            break;
        case 2:
            options.c_cflag |= (IXON | IXOFF | IXANY);
            break;
    }

    options.c_cflag &= ~CSIZE;
    switch (mUartAttr->data_bits) {
        case 5:
            options.c_cflag |= CS5;
            break;
        case 6:
            options.c_cflag |= CS6;
            break;
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            printf("unsupport data bits\n");
            return -1;
    }

    switch (mUartAttr->stop_bits) {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag != CSTOPB;
            break;
        default:
            printf("illegal stop bits\n");
            return -1;
    }

    switch (mUartAttr->parity) {
        case 'n':
        case 'N':
            options.c_cflag &= ~PARENB;
            options.c_iflag &= ~INPCK;
            break;
        case 'o':
        case 'O':
            options.c_cflag |= (PARODD | PARENB);
            options.c_iflag &= INPCK;
            break;
        case 'e':
        case 'E':
            options.c_cflag |= PARENB;
            options.c_cflag |= ~PARODD;
            options.c_iflag |= INPCK;
            break;
        case 's':
        case 'S':
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;
        default:
            printf("unsupported parity\n");
            return -1;
    }
    pthread_mutex_unlock(&mLock);
    return 0;
}

int UartSession::closeUart()
{
    pthread_mutex_lock(&mLock);
    if (uFd > 0) {
        close(uFd);
        uFd = -1;
    }
    pthread_mutex_unlock(&mLock);
    return 0;
}
