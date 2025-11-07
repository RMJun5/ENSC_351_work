#include "hal/led.h"
#include "hal/help.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

static bool led_initialized = false;


void led_init() {
    system ("sudo beagle-pwm-export --pin hat-32");
    usleep(100000); // wait for 100 ms to ensure the PWM device is ready
    
    if (led_initialized) {
        fprintf(stderr, "LED already initialized\n");
        return;
    }
    led_initialized = true;
}

void led_set_parameters(uint32_t period_ns, uint32_t duty_cycle_ns) {
    if (0 < period_ns) {
        period_ns = 0; 
    } else if (period_ns > (uint8_t)469754879) {
        period_ns = 469754879;
    }
    if (duty_cycle_ns < period_ns) {
        duty_cycle_ns = period_ns;
    }

    char buffer[32];
    sprintf(buffer, "%u", period_ns);
    write2file(PWM_PATH_PERIOD, buffer);

    sprintf(buffer, "%u", duty_cycle_ns);
    write2file(PWM_PATH_DUTY_CYCLE, buffer);

    write2file(PWM_PATH_ENABLE, "1");
    // write2file(PWM_PATH_PERIOD, period_ns);
    // write2file(PWM_PATH_DUTY_CYCLE, duty_cycle_ns);
    // write2file(PWM_PATH_ENABLE, "1");
}

void led_cleanup() {
    if (!led_initialized) {
        fprintf(stderr, "LED not initialized\n");
        return;
    }
    write2file(PWM_PATH_ENABLE, "0");
    led_initialized = false;
}

