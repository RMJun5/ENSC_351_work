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

// 1 second timer
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


int main() {

    // Initialize hardware & modules
    Period_init();
    led_init();
    led_set_parameters(1000000000, 500000); // 1s period, 50% duty

    sampler_init();     // initialize sampler once
    UDP_start();        // start UDP server
    Period_init();      // initialize period timer

    FILE* fileID = fopen("history.txt", "w");
    if (!fileID) {
        perror("fopen history.txt");
        return 1;
    }

    for (int i = 0; i < 1000; i++) {
        // wait 100ms
        usleep(100000);

        // get current reading
        double reading = sampler_getCurrentReading();
        fprintf(fileID, "%.3f, ", reading);

        // every 1s: rotate history and mark event
        static int counter = 0;
        counter++;
        if (counter >= 10) { // 10 * 0.1s = 1s
            Period_markEvent(PERIOD_EVENT_SAMPLE_FINAL);
            counter = 0;
        }
    }

    // Print statistics
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);
    printf("Light sensor statistics:\n");
    printf("  Num samples: %lld\n", sampler_getNumSamplesTaken());
    printf("  Avg period (ms): %.3f\n", stats.avgPeriodInMs);
    printf("  Min period (ms): %.3f\n", stats.minPeriodInMs);
    printf("  Max period (ms): %.3f\n", stats.maxPeriodInMs);

    // Cleanup
    fclose(fileID);
    UDP_stop();
    sampler_cleanup();
    Period_cleanup();
    led_cleanup();
    
    // // store history in a text file
    // // Make sure you change the permissions of the light_sampler directory ($ chmod a+rw light_sampler/)
    // FILE* fileID = fopen("history.txt", "w");
    // if (fileID == NULL) {
    //     perror("Error opening file");
    //     return 1;
    // }

    // for (int i = 0; i < 1000; i++) {
   
    //     int size = sampler_getHistorySize();

    //     Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
    //     // Read the light sensor
    //     fprintf(fileID, "%d, ", sampler_init());
    //     sleep((unsigned)0.1);   

    // }
    
    // printf("Light sensor thread statistics:\n");
    // printf("  Num samples: %d\n", stats.numSamples);
    // printf("  Min period (ms): %.3f\n", stats.minPeriodInMs);
    // printf("  Max period (ms): %.3f\n", stats.maxPeriodInMs);
    // printf("  Avg period (ms): %.3f\n", stats.avgPeriodInMs);

    // Period_markEvent(PERIOD_EVENT_MARK_SECOND);
    // Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);
    // Period_cleanup();
    // fclose(fileID);
    // led_cleanup();
    // return 0;
}