#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <reaction_timer.h>



static inline void rt_sleep_ms(long ms){
    struct timespec ts;
    ts.tv_sec = ms/1000;
    ts.tv_nsec = (ms%1000)* 1000000L;
    nanosleep(&ts, NULL);
}

static inline int rt_read_ch(int fd, int ch, uint32_t speed_hz) {
    if (fd < 0 || ch < 0 || ch > 7) return -1;
    uint8_t tx[3] = { (uint8_t)(0x06 | ((ch & 0x04) >> 2)), (uint8_t)((ch & 0x03) << 6), 0x00 };
    uint8_t rx[3] = {0};
    tx[0] = (uint8_t)(0x06 | ((ch & 0x04) >> 2));
    tx[1] = (uint8_t)((ch & 0x03) << 6);
    tx[2] = 0x00;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed_hz,
        .bits_per_word = 8,
        .cs_change = 0
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("SPI_IOC_MESSAGE");
        return -1;
    }

    int val = ((rx[1] & 0x0F) << 8) | rx[2]; //12-bit result
    return val;
}

int main(void){
const char* dev = "/dev/spidev0.0";
uint8_t mode = 0; // SPI mode 0
uint8_t bits = 8;
uint32_t speed = 250000;

    printf("Hello embedded world, from Richard!");

    printf("Now.... Get ready! \n");
    
    // open SPI
    int fd = open(dev, O_RDWR);
    if(fd<0) {perror(dev); return -1;}
    
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("SPI_IOC_WR_MODE"); close(fd); return 1;
    }
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("SPI_IOC_WR_BITS_PER_WORD"); close(fd); return 1;
    }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) == -1) {
        perror("SPI_IOC_WR_MAX_SPEED_HZ"); close(fd); return 1;
    }
  
    int up = rt_read_ch(fd,0,speed);
    int down = rt_read_ch(fd,1,speed);

    if(G_trigger="none" !=0) return 1;
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
    

    
    close(fd);
    return 0;   
} 