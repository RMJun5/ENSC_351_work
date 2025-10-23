#ifndef REACTION_TIMER_H
#define REACTION_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include "led.h"
#include "joystick.h"
#include "timing.h"

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

void cleanup (void){
    g_set_act_on(0);
    r_set_act_on(0);
}
/* Joystick calibration functions (from joystick.h) */
#ifdef JOY_CALIBRATION_ENABLED
void calibrate_joystick(joystick_t *joy);
#endif

#endif /* REACTION_TIMER_H */
