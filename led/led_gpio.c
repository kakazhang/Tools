#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define GPIO_LED_PATH "/dev/gpio_led"

void usage(void)
{
    printf("turn on led: GLed 1\n"
           "turn off led: GLed 0\n");
    exit(0);
}

int set_state(char state)
{
    int fd = open(GPIO_LED_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    int ret = write(fd, &state, sizeof(state));
    if (ret < 0) {
        perror("write");
    }

    close(fd);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        usage();

    int state = atoi(argv[1]);
    return set_state(state);
}
