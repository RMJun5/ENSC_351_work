#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Adjust these to your board's LED paths.
// List available LEDs with: ls /sys/class/leds
#define GREEN_TRIGGER_PATH    "/sys/class/leds/ACT/trigger"
#define GREEN_BRIGHTNESS_PATH "/sys/class/leds/ACT/brightness"

int main(){

    printf("Hello embedded world, from Richard!");

    printf("Now.... Get ready to rumble!");
    
    for (int i = 0; i<250; i++) {
        long seconds = 1;
    }
    return 0;   
} 