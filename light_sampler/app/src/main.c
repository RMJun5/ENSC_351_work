#include "periodTimer.h"
#include "hal/sensor.h" 
#include "hal/timing.h"
#include "hal/led.h"

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
    Period_init(); 

    led_init();
    led_set_parameters(1000000000, 500000); // 1 second period, 50% duty cycle
    
    // store history in a text file
    // Make sure you change the permissions of the light_sampler directory ($ chmod a+rw light_sampler/)
    FILE* fileID = fopen("history.txt", "w");
    if (fileID == NULL) {
        perror("Error opening file");
        return 1;
    }

    for (int i = 0; i < 1000; i++) {
   
    int size = getHistorySize();


        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        // Read the light sensor
        fprintf(fileID, "%d, ", sensor_read());
        sleep((unsigned)0.1);   

    }
    printf("Light sensor thread statistics:\n");
    printf("  Num samples: %d\n", stats.numSamples);
    printf("  Min period (ms): %.3f\n", stats.minPeriodInMs);
    printf("  Max period (ms): %.3f\n", stats.maxPeriodInMs);
    printf("  Avg period (ms): %.3f\n", stats.avgPeriodInMs);

    Period_markEvent(PERIOD_EVENT_MARK_SECOND);
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);
    Period_cleanup();
    fclose(fileID);
    led_cleanup();
    return 0;
}