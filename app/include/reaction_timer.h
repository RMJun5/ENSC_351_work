#ifndef REACTION_TIMER_H
#define REACTION_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <joystick.h>
#include <led.h>

/* default joystick axes for reaction timer */
#define JOY_DEFAULT_AXIS_X 0
#define JOY_DEFAULT_AXIS_Y 1

/*static struct timespec getTimeinms()
{
timespec
clock_gettime(CLOCK_REALTIME, &spec);
long long seconds = spec.tv_sec;
long long nanoSeconds = spec.tv_nsec;
long long milliSeconds = seconds * 1000
+ nanoSeconds / 1000000;
return milliSeconds;
}
*/
// Sleep in milliseconds 
/* Sleep in milliseconds */
static inline void sleep_ms(long long ms) {
    const long long NS_PER_MS = 1000LL * 1000LL;
    const long long NS_PER_SECOND = 1000000000LL;
    long long delayNs = ms * NS_PER_MS;
    int seconds = (int)(delayNs / NS_PER_SECOND);
    int nanoseconds = (int)(delayNs % NS_PER_SECOND);
    struct timespec reqDelay = { .tv_sec = seconds, .tv_nsec = nanoseconds };
    nanosleep(&reqDelay, NULL);
}

/* Millisecond wall-clock using CLOCK_REALTIME (not used for intervals in main; kept for reference) */
static inline long long getTimeInMs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = (long long)spec.tv_sec;
    long long nanoSeconds = (long long)spec.tv_nsec;
    long long milliSeconds = seconds * 1000LL + nanoSeconds / 1000000LL;
    return milliSeconds;
}
/* Flash an LED function pointer N times in a total of duration_ms */
static inline void flash_led(int (*led_on)(int), int times, int duration_ms) {
    int on_time = duration_ms / (2 * times);
    for (int i = 0; i < times; i++) {
        led_on(1);
        sleep_ms(on_time);
        led_on(0);
        sleep_ms(on_time);
    }
}

/* Joystick calibration functions (from joystick.h) */
#ifdef JOY_CALIBRATION_ENABLED
void calibrate_joystick(joystick_t *joy);
#endif

#endif /* REACTION_TIMER_H */
