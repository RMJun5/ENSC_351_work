#define _GNU_SOURCE
#include "hal/periodTimer.h"
#include "hal/sampler.h" 
#include "hal/UDP.h"
#include "hal/encoder.h"
#include "hal/led.h"
#include "hal/encoder.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

Period_statistics_t stats;
static bool running = false;
static double samples[1000];

/**
 * @brief Check if 1 second has elapsed
 * 
 * @return true
 * @return false 
 */
bool timeElapsed(){

    struct timespec current, prev;
    prev.tv_sec = 0;
    prev.tv_nsec = 0;
    current.tv_sec = 0;
    current.tv_nsec = 0;

    clock_gettime(CLOCK_MONOTONIC, &current);

    if (current.tv_nsec == 0 && current.tv_sec == 0) {
        prev = current;
        return false;
    }
    
    double time = (current.tv_sec - prev.tv_sec) + (current.tv_nsec - prev.tv_nsec) / 1e9;
    if (time > 1.0) { return true;}
    return false;
}

/**
 * @brief Update the speed of the LED when the encoder is turned
 * 
 * @param encoder_bits the bits of GPIO pins 16 and 17 of the encoder
 */
void update_led(int encoder_bits) {
    static int pwm_period_ns = 1000000000; // default 1s
    static int pwm_duty_ns = 500000000;    // 50%

    switch (encoder_bits) {
        case 0: pwm_period_ns = 800000000; break; // faster blink
        case 1: pwm_period_ns = 600000000; break;
        case 2: pwm_period_ns = 400000000; break;
        case 3: pwm_period_ns = 200000000; break; // very fast
    }

    led_set_parameters(pwm_period_ns, pwm_duty_ns);
}

/**
 * @brief Count the number of light dips in the array
 * 
 * @param samples the array
 * @param n the length of the array
 * @return int the number of dips
 */
int count_light_dips(double *samples, int n) {
    int dips = 0;
    for (int i = 1; i < n; i++) {
        if (samples[i] < samples[i - 1] * 0.8)
            dips++;
    }
    return dips;
}



int main() {

    // Initialize hardware & modules
    Period_init();      // initialize period timer
    printf("%s", "Initialized the period timer\n");

    encoder_init();
    printf("%s", "Initialized the encoder\n");

    led_init();
    printf("%s", "Initialized the LED\n");

    sampler_init();
    printf("%s", "Initialized the sampler\n");

    UDP_start();
    printf("%s", "Started the UDP server\n");


    int i = 0; // index
    int dips = 0;
    double prev = 0;    // int
    double current = 0; // int

    running = true;
    
    while (running) {

        // get current light level
        double reading = sampler_getCurrentReading();
        samples[i] = reading;
        i++;


        // if the index of the array is greater than 1000, reset the index
        if (i >= 1000) {
            i = 0;
        }

        // if the current reading is less than 80% of the previous reading, add one to 
        // the dips counter (because the light dips hahaha)
        if (current < prev * 0.8) {
            dips++;
        }
        prev = current;

        // int gpio16 = read_encoder();
        // int gpio17 = read_encoder();
        int bits = read_encoder(); // returns the bits of encoder

        update_led(bits);

        if (timeElapsed(1000000000)) {
            printf("light level: %f\nDips in 1s: %d\nEncoder: %d", current, dips, bits);
            Period_markEvent(PERIOD_EVENT_SAMPLE_FINAL);
            dips = 0;
        }

        //UDP_poll();
        usleep(10000);

    }
    //  for (int i = 0; i < 1000; i++) {

    //     int val = read_encoder();
    //     printf("Encoder bits: %d (GPIO16=%d, GPIO17=%d)\n", val, (val >> 1) & 1, val & 1);
    //     // wait 100ms


    //     // get current reading
    //     //double reading = sampler_getCurrentReading();

    //     // every 1s: rotate history and mark event
    //     static int counter = 0;
    //     counter++;
    //     if (counter >= 10) { // 10 * 0.1s = 1s
    //         Period_markEvent(PERIOD_EVENT_SAMPLE_FINAL);
    //         counter = 0;
    //     }
    // }

    // // Print statistics
    // Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);
    // printf("Light sensor statistics:\n");
    // printf("  Num samples: %lld\n", sampler_getNumSamplesTaken());
    // printf("  Avg period (ms): %.3f\n", stats.avgPeriodInMs);
    // printf("  Min period (ms): %.3f\n", stats.minPeriodInMs);
    // printf("  Max period (ms): %.3f\n", stats.maxPeriodInMs);

    // Cleanup
    UDP_stop();
    sampler_cleanup();
    Period_cleanup();
    led_cleanup();
    clean_encoder();   
    running = false; 
    
    return 0;
}