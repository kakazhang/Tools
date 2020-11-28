#ifndef COMMON_H
#define COMMON_H

typedef struct {
    int type;
    int timeout;
} Attr;

typedef struct {
    Attr attr;
    char *ip;
    int port;
} TcpAttr;

#endif
