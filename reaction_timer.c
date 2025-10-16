#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <led.h>
#include <joystick.h>



int main(void){
const char* dev = "/dev/spidev0.0";
uint8_t mode = 0; // SPI mode 0
uint8_t bits = 8;
uint32_t speed = 250000;

    joy_open(dev, speed, mode)

    printf("Hello embedded world, from Richard!");

    printf("Now.... Get ready! \n");
    
    
    int up = joy_read(fd,0,speed);
    int down = joy_read (fd,1,speed);

    if(G_trigger_path ="none" !=0) return 1;
    if((R_trigger_path="none")!=0 ) return 1; 

    for (int i=0; i<4;++i){
        // Green LED on for 250 m/s then off
        if (g_set_act_on(1)!=0) return 1;
        rt_sleep_ms(250);
        if (g_set_act_on(0)!=0) return 1;

        // Red LED on for 250 m/s then off
        if (r_set_on(1)!=0) return 1;
        rt_sleep_ms(250);
        if (r_set_on(0)!=0) return 1;
    }
    
    if 

    close(fd);
    return 0;   
} 