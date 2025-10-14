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

// Build "/sys/class/leds/<name>/<leaf>" into buf
static inline int rt_build_led_path_(char *buf, size_t buflen,
                                     const char *name, const char *leaf) {
    if (!buf || !name || !leaf) return -1;
    int n = snprintf(buf, buflen, "/sys/class/leds/%s/%s", name, leaf);
    return (n < 0 || (size_t)n >= buflen) ? -1 : 0;
}

// --- Public API ---

// Set LED trigger, e.g., "none", "heartbeat", "timer", ...
static inline int rt_led_set_trigger(const char *led_name, const char *trigger) {
    char path[RT_MAX_PATH];
    if (rt_build_led_path_(path, sizeof(path), led_name, "trigger") != 0) return -1;
    return rt_write_sysfs_(path, trigger);
}

// Turn LED on/off (1 or 0) via brightness
static inline int rt_led_set_on(const char *led_name, int on) {
    char path[RT_MAX_PATH];
    if (rt_build_led_path_(path, sizeof(path), led_name, "brightness") != 0) return -1;
    return rt_write_sysfs_(path, on ? "1" : "0");
}

// Convenience: ensure manual control, then flash ON for `on_ms`, then OFF.
static inline int rt_led_flash_ms(const char *led_name, long on_ms) {
    if (rt_led_set_trigger(led_name, "none") != 0) return -1;
    if (rt_led_set_on(led_name, 1) != 0) return -1;
    rt_sleep_ms(on_ms);
    if (rt_led_set_on(led_name, 0) != 0) return -1;
    return 0;
}



#endif