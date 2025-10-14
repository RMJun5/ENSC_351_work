#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <reaction_timer.h>

// List available LEDs with: ls /sys/class/leds
#define LED_GREEN "/ACT"
#define LED_RED "/PWR"

int main(){

    printf("Hello embedded world, from Richard!");

    printf("Now.... Get ready! \n");
    
    if(rt_led_set_trigger(LED_GREEN,"none") !=0) return 1;
    if(rt_led_set_trigger(LED_RED,"none")!=0 ) return 1; 

    for (int i=0; i<4;++i){
        // Green LED on for 250 m/s then off
        if (rt_led_set_on(LED_GREEN, 1)!=0) return 1;
        rt_sleep_ms(250);
        if (rt_led_set_on(LED_GREEN, 0)!=0) return 1;
        
        // Red LED on for 250 m/s then off
        if (rt_led_set_on(LED_RED,1)!=0) return 1;
        rt_sleep_ms(250);
        if (rt_led_set_on(LED_RED,0)!=0) return 1;
    }
    
    return 0;   
} 