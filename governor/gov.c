#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define CPUX_PATH "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor"

int get_cpu_num() {
    char buf[16] = {0};
    int num;
    FILE* fp = popen("cat /proc/cpuinfo |grep processor|wc -l", "r");
    if(fp) {
        fread(buf, 1, sizeof(buf) - 1, fp);
        pclose(fp);
    }
    num = atoi(buf);
    if(num <= 0){
        num = 1;
    }
    return num;
}

int switch_governor(char *gov)
{
    int i, cpus;
    int count;
    char buf[128] = {0};

    if (!gov) {
        printf("governor is null\n");
        return -1;
    }

    cpus = get_cpu_num();
    for (i = 0; i < cpus; i++) {
        count = snprintf(buf, sizeof(buf), CPUX_PATH, i);
        int fd = open(buf, O_RDWR);
        if (fd < 0) {
            perror("open");
            close(fd);
            return -1;
        }

        count = write(fd, gov, strlen(gov)+1);
        if (count < 0) {
            perror("write");
            close(fd);
            return -1;
        }
    }

    return 0;
}

void usage(void)
{
    printf("GOV $governor\n");
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc != 2) usage();

    switch_governor(argv[1]);

    return 0;
}
