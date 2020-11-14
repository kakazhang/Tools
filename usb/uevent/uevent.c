#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <linux/netlink.h>

#include "uevent.h"

static int fd = -1;

void parse(const char *msg, struct uevent *event)
{
    event->action = "";
    event->path   = "";
    event->subsystem = "";
    event->aoa_state = "";

    while (*msg) {
        if (!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
            event->action = msg;
        } else if (!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            event->path = msg;
        } else if (!strncmp(msg, "SUBSYSTEM", 10)) {
            msg += 10;
            event->subsystem = msg;
        } else if (!strncmp(msg, "AOA_STATE=", 10)) {
            msg+=10;
            event->aoa_state = msg;
        }

        while (*msg++) ;
    }
}

int uevent_init() {
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(s < 0)
        return 0;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return 0;
    }

    fd = s;
    return (fd > 0);
}

int uevent_next_event(char* buffer, int buffer_length)
{
    while (1) {
        struct pollfd fds;
        int nr;

        fds.fd = fd;
        fds.events = POLLIN;
        fds.revents = 0;
        nr = poll(&fds, 1, -1);

        if(nr > 0 && (fds.revents & POLLIN)) {
            int count = recv(fd, buffer, buffer_length, 0);
            return count;
        }
    }

    // won't get here
    return 0;
}
