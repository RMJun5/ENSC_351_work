#include "hal/led.h"
#include "hal/help.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
const uint32_t MAX_PERIOD = 469754879;
#define MIN_FREQ 0
#define MAX_FREQ 500
#define INIT_FREQ 10

static bool led_initialized = false;

/**
 * @brief Initializes the led. Should only be called once
 */
void led_init() {
    if (led_initialized) {
        fprintf(stderr, "LED already initialized\n");
        return;
    }

    // Export the PWM pin
    system ("sudo beagle-pwm-export --pin hat-32");
    usleep(100000); // wait for 100 ms to ensure the PWM device is ready
    
    led_initialized = true;
}

/**
 * @brief Sets the LED parameters and enables the LED
 * 
 * @param period_ns the period in nanoseconds
 * @param duty_cycle_ns the duty cycle in nanoseconds
 */
void led_set_parameters(uint32_t period_ns, uint32_t duty_cycle_ns) {
   
    if (!led_initialized) {
        fprintf(stderr, "LED not initialized\n");
        return;
    }


    // The original if (0 < period_ns) { period_ns = 0; } makes no sense.
    // Correct logic is: ensure valid nonzero period and duty cycle < period.
    if (period_ns <= 0) {
       period_ns = 0;
    }

    // Clamp period to hardware max if necessary
    if (period_ns > MAX_PERIOD) {
        period_ns = MAX_PERIOD;
    }

    // Clamp duty cycle to period
    if (duty_cycle_ns > period_ns) {
        duty_cycle_ns = period_ns;
    }


    char buffer[32];

    // Write period and duty cycle to the file
    snprintf(buffer, sizeof(buffer), "%u", period_ns);
    write2file(PWM_PATH_PERIOD, buffer);
    // sprintf(buffer, "%u", period_ns);
    // write2file(PWM_PATH_PERIOD, buffer);

    snprintf(buffer, sizeof(buffer), "%u", duty_cycle_ns);
    write2file(PWM_PATH_DUTY_CYCLE, buffer);
    // sprintf(buffer, "%u", duty_cycle_ns);
    // write2file(PWM_PATH_DUTY_CYCLE, buffer);


    // Enable the PWM
    write2file(PWM_PATH_ENABLE, "1");
    // write2file(PWM_PATH_PERIOD, period_ns);
    // write2file(PWM_PATH_DUTY_CYCLE, duty_cycle_ns);
    // write2file(PWM_PATH_ENABLE, "1");
}

/**
 * @brief Cleans up the led
 * 
 */
void led_cleanup() {
    if (!led_initialized) {
        fprintf(stderr, "LED not initialized\n");
        return;
    }
    write2file(PWM_PATH_ENABLE, "0");
    led_initialized = false;
}