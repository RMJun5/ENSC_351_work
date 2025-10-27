#define _POSIX_C_SOURCE 200809L 
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "hal/timing.h"

double nanotoms (int ns){
    double ms = (double)ns / 1000000.0;
    return ms;
}


long long getTimeInMs(void) {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = (long long)spec.tv_sec;
    long long nanoSeconds = (long long)spec.tv_nsec;
    long long milliSeconds = seconds * 1000LL + nanoSeconds / 1000000LL;
    return milliSeconds;
}
long long runtime(long long present, long long start){
    long long period = present - start;
    return period;
}

void sleep_ms(long long ms){
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    
    long long delayNs = ms * NS_PER_MS;
    int seconds = (int)(delayNs / NS_PER_SECOND);
    int nanoseconds = (int)(delayNs % NS_PER_SECOND);
    struct timespec reqDelay = { .tv_sec = seconds, .tv_nsec = nanoseconds };
    nanosleep(&reqDelay,(struct timespec *)NULL);
}