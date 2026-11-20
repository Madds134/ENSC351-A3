#include "hal/timing.h"
#include <time.h>

void Timing_sleepMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

long long Timing_getTimeMs(void)
{
    const long long MS_PER_SECOND = 1000;
    const long long NS_PER_MS = 1000 * 1000;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * MS_PER_SECOND + now.tv_nsec / NS_PER_MS;
}
