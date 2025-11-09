#define _GNU_SOURCE
#include "hal/periodTimer.h"
#include "hal/sampler.h" 
#include "hal/UDP.h"
#include "hal/encoder.h"
#include "hal/led.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>


Period_statistics_t stats;
static double samples[1000];

#define MIN_FREQ 0      // 0 Hz
#define MAX_FREQ 500    // 500 Hz
#define INIT_FREQ 10    // 10 Hz

#define MIN_PERIOD_NS 200000000  // 0.2 s, adjust for your hardware
#define MAX_PERIOD_NS 1000000000 // 1 s, adjust for your hardware

// static Sampler samp;

/**
 * @brief Check if 1 second has elapsed
 * 
 * @return true
 * @return false 
 */
bool timeElapsed(void) {
    static struct timespec prev = {0, 0};
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);

    pthread_mutex_lock(&lock);

    if (prev.tv_sec == 0 && prev.tv_nsec == 0) {
        prev = current;  // first call
        pthread_mutex_unlock(&lock);
        return false;
    }

    double elapsed = (current.tv_sec - prev.tv_sec) +
                     (current.tv_nsec - prev.tv_nsec) / 1e9;
    if (elapsed < 0) elapsed = 0;

    if (elapsed >= 1.0) {
        prev = current;  // reset timer
        pthread_mutex_unlock(&lock);
        return true;
    }

    pthread_mutex_unlock(&lock);
    return false;
}

/**
 * @brief Read the bits of GPIO pins 22 and 27 of the encoder and return 0,1,2,3
 * 
 * @return int 
 */
int read_encoder_step_debounced(void) {
    static int last_bits = 0;
    int bits = read_encoder();  // returns 0,1,2,3

    int step = 0;
    int delta = bits - last_bits;

    if (delta == 1 || delta == -3) step = 1;       // clockwise
    else if (delta == -1 || delta == 3) step = -1; // counter-clockwise
    // else step = 0; // any jump that is not 1 or -1 is ignored

    last_bits = bits;
    return step;
}

/**
 * @brief Update the speed of the LED when the encoder is turned
 * 
 * @param encoder_bits the bits of GPIO pins 22 and 27 of the encoder
 */
void update_led() {

    static int freq_hz = INIT_FREQ;  // start at 10 Hz
   static int last_freq_hz = 0;    // track last applied frequency

    // read rotary encoder step: +1 or -1
    int step = read_encoder_step_debounced();

    // Update frequency
    freq_hz += step;
    if (freq_hz < MIN_FREQ) freq_hz = MIN_FREQ;
    if (freq_hz > MAX_FREQ) freq_hz = MAX_FREQ;

    // Only update LED if frequency changed
    if (freq_hz != last_freq_hz) {
        int period_ns = (freq_hz > 0) ? 1000000000 / freq_hz : 1000000000; // 0Hz -> very long period
        int duty_ns   = period_ns / 2;  // 50% duty

        led_set_parameters(period_ns, duty_ns);
        last_freq_hz = freq_hz;

        printf("LED frequency: %d Hz, period: %d ns, duty: %d ns\n",
               freq_hz, period_ns, duty_ns);
    }
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

    Sampler* s = sampler_getHandle();
    printf("initialized = %d\n", s->initialized);

    UDP_start();
    printf("%s", "Started the UDP server\n");


    int i = 0; // index
    int dips = 0;
    double current = 0; // int
    
    while (s->initialized) {

        // get current light level
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        double reading = sampler_getCurrentReading();
        current = reading;
        samples[i] = current;
        i++;
        i = (i + 1) % 1000; // wrap around safely

        // i = (i + 1) % 1000;
        // samples[i] = current;

        // if the current reading is less than 80% of the previous reading, add one to 
        // the dips counter (because the light dips hahaha)
        double avg = sampler_getAverageReading();
        if (current < avg - 0.1) {
            dips++;
        }

        // int bits = read_encoder(); // returns the bits of encoder
        // update_led(bits);
        update_led();

        if (timeElapsed()) {
            printf("Names: Richard Kim and Kirstin Horvat \n");
            //printf("light level: %f\nDips in 1s: %d\nEncoder: %d\n", current, dips, bits);
            Period_markEvent(PERIOD_EVENT_SAMPLE_FINAL);
            Period_statistics_t stats, lightStats;
            Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_FINAL, &stats);
            Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &lightStats);
            printf("Light stats: numSamples=%d, avg=%.3fms, min=%.3fms, max=%.3fms\n",
                    lightStats.numSamples, lightStats.avgPeriodInMs, lightStats.minPeriodInMs, lightStats.maxPeriodInMs);
            printf("Event stats: numSamples=%d, avg=%.3fms, min=%.3fms, max=%.3fms\n",
                    stats.numSamples, stats.avgPeriodInMs, stats.minPeriodInMs, stats.maxPeriodInMs);

            sampler_moveCurrentDataToHistory();
            dips = 0;
        }

        usleep(1000); // sleep for 1ms

    }

    // Cleanup
    UDP_stop();
    sampler_cleanup();
    clean_encoder(); 
    led_cleanup();
    Period_cleanup();  
    
    return 0;
}