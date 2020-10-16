#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define PATH_CACHE "/proc/sys/vm/drop_caches"

void usage()
{
    printf("DROP 1 for free page cache\n"
           "DROP 2 for free slab cache\n"
           "DROP 3 for free page and slab cache\n");
    exit(-1);
}

int main(int argc, char **argv)
{
    int ret = 0;

    if (argc != 2)
        usage();

    int fd = open(PATH_CACHE, O_WRONLY);
    if (fd < 0) {
        perror("open");
        ret = -1;
        goto err;
    }

    int action = atoi(argv[1]);
    if (action < 1 || action > 3) {
        printf("illegal value\n");
        ret = -2;
        goto err;
    }

    printf("write %d\n", action);
    ret = write(fd, (void *)argv[1], strlen(argv[1]));
    if (ret < 0) {
        perror("write");
        ret = -3;
        goto err;
    }
err:
    if (fd > 0)
        close(fd);
    return ret;
}
