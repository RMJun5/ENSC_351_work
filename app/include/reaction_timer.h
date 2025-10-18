#ifndef REACTION_TIMER_H
#define REACTION_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include <joystick.h>
#include <led.h>

/* default joystick axes for reaction timer */
#define JOY_DEFAULT_AXIS_X 0
#define JOY_DEFAULT_AXIS_Y 1

/* ADC resolution for joystick */
#define JOY_ADC_BITS 12  /* 12-bit ADC assumed */

/* Sleep in milliseconds */
static inline void sleep_ms(int ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
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
