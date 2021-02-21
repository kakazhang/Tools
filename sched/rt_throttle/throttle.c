#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>

void test_throttle()
{
    unsigned long long i = 0;
    struct sched_param param;
    param.sched_priority = 60;

    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    int ret = sched_setscheduler(0, SCHED_RR, &param);
    if (ret < 0) {
        perror("sched_setscheduler");
    }
    pthread_mutex_lock(&lock);
    while (1) {
        i++;
    }
    pthread_mutex_unlock(&lock);
}

int main(int argc, char** argv)
{
    test_throttle();

    return 0;
}
