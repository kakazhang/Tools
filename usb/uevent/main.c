#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include "uevent.h"

#define BUFLEN 1024

#define ACTION_ADD      "add"
#define ACTION_REMOVE   "remove"

#define BLOCK_PATH "/block/sda/sda1"
#define DEV_PATH    "/dev/sda1"
#define MOUNT_PATH "/mnt/sda1"

int do_mount(struct uevent *event)
{
    int ret;

    printf("%s %s\n", event->action, event->path);
    if (access(MOUNT_PATH, F_OK)) {
        ret = mkdir(MOUNT_PATH, 0666);
        if (ret) {
            perror("mkdir");
            return -1;
        }
    }

    ret = mount(DEV_PATH, MOUNT_PATH, "vfat", 0, NULL);
    if (ret) {
        perror("mount");
        return -1;
    }

    return 0;
}


int do_umount(struct uevent *event)
{
    printf("%s %s\n", event->action, event->path);
    return umount(MOUNT_PATH);
}

#define PATH_AOA "/dev/aoa"
#define BUF_SIZE  0x400

volatile unsigned char aoa_ready = 0;
void show(char *buf, int count)
{
    int i;
    for (i = 0; i < count; i++)
        printf("%d ", buf[i]);
    printf("\n");
}

static void* aoa_read(void *args)
{
    char buf[BUF_SIZE] = {0};
    int fd = open(PATH_AOA, O_RDWR);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    while (aoa_ready) {
        int count = read(fd, buf, sizeof(buf));
        if (count > 0) {
            printf("read data\n");
            show(buf, count);
        }
    }

    return NULL;
}

int main(int argc, char** argv)
{
    int result = 0;
    char buf[BUFLEN] = {0};

    result = uevent_init();
    if (!result) {
        printf("uevent init failed\n");
        return -1;
    }

    int afd = -1;
    while (1) {
        struct uevent event;

        result = uevent_next_event(buf, BUFLEN);
        if (result > 0) {
            parse(buf, &event);
            if (!strcmp(event.action, ACTION_ADD) && strstr(event.path, BLOCK_PATH)) {
                do_mount(&event);
            } else if (!strcmp(event.action, ACTION_REMOVE) && strstr(event.path, BLOCK_PATH)) {
                do_umount(&event);
            } else if (!strcmp(event.aoa_state, "CONNECTED")) {
                aoa_ready = 1;
                pthread_t thread;
                pthread_create(&thread, NULL, aoa_read, NULL);
                printf("aoa connected\n");
            } else if (!strcmp(event.aoa_state, "DISCONNECTED")) {
                aoa_ready = 0;
                printf("aoa disconnected\n");
            }
        }
    }

    return 0;
}
