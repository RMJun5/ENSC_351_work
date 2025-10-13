#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <reaction_timer.h>

// List available LEDs with: ls /sys/class/leds
int set_act_trigger(const char *trigger)
{
    FILE *f = fopen('G_LED_TRIGGER_PATH', "w");
    if (f == NULL) {
        perror("open trigger");
        return -1;
    }

    int n = fprintf(f, "%s", trigger);  // "none", "heartbeat", "timer", ...
    if (n <= 0) {
        perror("write trigger");
        fclose(f);
        return -1;
    }

    if (fclose(f) != 0) {
        perror("close trigger");
        return -1;
    }
    return 0;
}
int main(){

    printf("Hello embedded world, from Richard!");

    printf("Now.... Get ready to rumble!");
    
    for (int i = 0; i<250; i++) {
        long seconds = 1;
    }
    return 0;   
} 