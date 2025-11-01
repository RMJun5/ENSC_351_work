#include "periodTimer.h"
#include "hal/sensor.h" 
#include "hal/timing.h"
#include "hal/led.h"

#include <stdio.h>
#include <string.h>


int main() {

    struct timespec reqDelay;
    reqDelay.tv_sec = 0;
    reqDelay.tv_nsec = 250000000;

    led_init();
    led_set_parameters(1000000000, 500000); // 1 second period, 50% duty cycle
    
    // store history in a text file
    // Make sure you change the permissions of the light_sampler directory ($ chmod a+rw light_sampler/)
    FILE* fileID = fopen("history.txt", "w");
    if (fileID == NULL) {
        perror("Error opening file");
        return 1;
    }
   
    int size = getHistorySize();


    // Read the light sensor
    for (int n = 0, n<10, n++) {
        fprintf(fileID, "%d\n", sensor_read());
        nanosleep(nanotoms(&reqDelay), NULL);
    }

    fclose(fileID);
    led_cleanup();
    return 0;
}