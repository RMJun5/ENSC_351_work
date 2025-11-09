#define _GNU_SOURCE
#include "hal/periodTimer.h"
#include "hal/sampler.h" 
#include "hal/UDP.h"
#include "hal/encoder.h"
#include "hal/led.h"
#include "hal/encoder.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

Period_statistics_t stats;
static double samples[1000];
static double curr_hz = 10.0;                 // start at 10 Hz
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

    if (elapsed >= 1.0) {
        prev = current;  // reset timer
        pthread_mutex_unlock(&lock);
        return true;
    }

    pthread_mutex_unlock(&lock);
    return false;
}

/**
 * @brief Update the speed of the LED when the encoder is turned
 * 
 * @param encoder_bits the bits of GPIO pins 16 and 17 of the encoder
 */
void update_led(int encoder_bits) {
    static const double MIN_HZ = 0.0;
    static const double MAX_HZ = 500.0;
     static int last_bits = -1;                    // previous encoder state
    int delta = 0;
    if (last_bits != -1 && encoder_bits != last_bits) {
        int transition = ((last_bits & 0x3) << 2) | (encoder_bits & 0x3);
        switch (transition) {
            case 0b0001: case 0b0111: case 0b1110: case 0b1000:
                delta = +1;  // clockwise
                break;
            case 0b0010: case 0b1011: case 0b1101: case 0b0100:
                delta = -1;  // counter-clockwise
                break;
            default:
                delta = 0;   // invalid/no change
                break;
        }
    }
    last_bits = encoder_bits;

    if (delta != 0) {
        double new_hz = curr_hz + delta;

        // clamp between 0 and 500 Hz
        if (new_hz < MIN_HZ) new_hz = MIN_HZ;
        if (new_hz > MAX_HZ) new_hz = MAX_HZ;

        // only update PWM if frequency actually changed
        if (new_hz != curr_hz) {
            curr_hz = new_hz;

            if (curr_hz > 0.0) {
                unsigned int period_ns = (unsigned int)(1e9 / curr_hz);
                unsigned int duty_ns   = period_ns / 2;   // 50% duty
                led_set_parameters(period_ns, duty_ns);
            } else {
                // turn off LED cleanly at 0 Hz
                led_set_parameters(0, 0);
            }

            printf("LED frequency: %.1f Hz\n", curr_hz);
        }
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
    luint32_t start_period = (uint32_t)(1e9 / curr_hz);
    uint32_t start_duty   = start_period / 2;
    led_set_parameters(start_period, start_duty);
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

        
        // if the index of the array is greater than 1000, reset the index
        // if (i >= 1000) {
        //     i = 0;
        // }

        // if the current reading is less than 80% of the previous reading, add one to 
        // the dips counter (because the light dips hahaha)
        double avg = sampler_getAverageReading();
        if (current < avg - 0.1) {
            dips++;
        }

        // int gpio16 = read_encoder();
        // int gpio17 = read_encoder();
        int bits = read_encoder(); // returns the bits of encoder
        update_led(bits);

        if (timeElapsed()) {
            printf("light level: %f\nDips in 1s: %d\nEncoder: %d\n", current, dips, bits);
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

        //UDP_poll();
        usleep(1000); // sleep for 1ms

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
    clean_encoder(); 
    led_cleanup();
    Period_cleanup();  
    
    return 0;
}