#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>   // close()
#include <reaction_timer.h>



int main(void){
const char* green = "/ACT";
const char* red = "/PWR";
const char* dev = "/dev/spidev0.0";
uint8_t mode = 0; // SPI mode 0
uint8_t bits = 8;
uint32_t speed = 250000;

    printf("Hello embedded world, from Richard!");

    printf("Now.... Get ready! \n");
    
    // open SPI
    int fd = rt_spi_open_device(dev, mode, bits, speed);
    if (fd < 0) {perror("open"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode)==-1){perror("mode"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) { perror("bpw"); return 1; }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1){ perror("speed"); return 1; }


    if(rt_led_set_trigger(green,"none") !=0) return 1;
    if(rt_led_set_trigger(red,"none")!=0 ) return 1; 

    for (int i=0; i<4;++i){
        // Green LED on for 250 m/s then off
        if (rt_led_set_on(green, 1)!=0) return 1;
        rt_sleep_ms(250);
        if (rt_led_set_on(green, 0)!=0) return 1;
        
        // Red LED on for 250 m/s then off
        if (rt_led_set_on(red,1)!=0) return 1;
        rt_sleep_ms(250);
        if (rt_led_set_on(red,0)!=0) return 1;
    }
    
    rt_wait_joystick()
    
    close(fd);
    return 0;   
} 