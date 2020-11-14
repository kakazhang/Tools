#ifndef UEVENT_H
#define UEVENT_H

struct uevent {
    const char *action;
    const char *path;
    const char *subsystem;
    const char *aoa_state;
};

int uevent_init();

int uevent_next_event(char* buffer, int buffer_length);

void parse(const char *msg, struct uevent *event);
#endif
