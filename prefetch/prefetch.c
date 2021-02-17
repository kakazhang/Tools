#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define N 5

static char *files[N] = {
    "/lib/libc-2.23.so",
    "/lib/ld-2.23.so",
    "/usr/share/fonts/ttf/PangMenZhengDao.ttf",
    "/opt/qt5.5.1/apps/QWebBrowser/QWebBrowser",
    "/opt/qt5.5.1/apps/QPictureViewer/QPictureViewer"
};

static void readahead_file(char *file)
{
    struct stat st;
    size_t size;

    stat(file, &st);
    size = st.st_size;

    int fd = open(file, O_RDWR);
    if (fd > 0) {
#ifdef READAHEAD
        int ret = readahead(fd, 0, size);
        if (ret) {
            perror("readahead");
        }
        printf("readahead %s\n", file);
#else
        void *addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED)
            perror("mmap");
        else
            printf("mmap for %s success\n", file);
#endif
        close(fd);
    }
}

static void readahead_files()
{
    int i;
    int count = N;

    for (i = 0; i < count; i++)
        readahead_file(files[i]);
}

int main(int argc, char** argv)
{
    readahead_files();

    while (1)
        sleep(60);

    return 0;
}
