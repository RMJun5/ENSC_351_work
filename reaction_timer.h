#ifndef REACTION_TIMER_H
#define REACTION_TIMER_H

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef RT_MAX_PATH
    #define RT_MAX_PATH 256
#endif

#define G_LED_TRIGGER_PATH "/sys/class/leds/ACT/trigger"  // "none","heartbeat","timer"
#define G_LED_BRIGHTNESS_PATH "/sys/class/leds/ACT/brightness" // 0 and 1
#define R_LED_TRIGGER_PATH "/sys/class/leds/PWR/trigger" // 
#define R_LED_BRIGHTNESS_PATH "/sys/class/leds/PWR/brightness"

// timing using nanosleep
static inline void rt_sleep_ms(long ms){
    struct timespec ts;
    ts.tv_sec = ms/1000;
    ts.tv_nsec = (ms%1000)* 1000000L;
    nanosleep(&ts, NULL);
}

// writing a string to sysfile
static inline int rt_write_sysfs_(const char *path, const char *s){
    FILE *f = fopen(path, "w");
    if(!f) {perror(path); return -1;}
    if (fprintf(f, "%s", s) <=0) {perror("fprintf"); fclose(f); return -1;}
    if (fclose(f) !=0){perror("fclose"); return -1;}
    return 0;
}




#endif