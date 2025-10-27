
#include "hal/sensor.h" 
#include "hal/timing.h"
#include <stdio.h>
#include <string.h>


int main() {

    struct timespec reqDelay;
    reqDelay.tv_sec = 0;
    reqDelay.tv_nsec = 250000000;

    // store history in a text file
    // Make sure you change the permissions of the light_sampler directory ($ chmod a+rw light_sampler/)
    FILE* fileID = fopen("history.txt", "w");
    if (fileID == NULL) {
        perror("Error opening file");
        return 1;
    }
    int n = 0;
    getHistorySize();

    // Read the light sensor
    while(n < 10) {
        fprintf(fileID, "%d\n", sensor_read());
        nanosleep(nanotoms(&reqDelay), NULL);
        n++;
    }

    fclose(fileID);
}