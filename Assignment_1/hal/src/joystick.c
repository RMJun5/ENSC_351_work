#define _POSIX_C_SOURCE 200809L 
#include "joystick.h"
#include "timing.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define SPI_SPEED 250000

static int min_x = 4095; static int min_y = 4095;
static int max_x = 0; static int max_y = 0;

int joy_read_ch(int fd, int ch, uint32_t speed_hz) {
    if (ch < 0 || ch > 7) return -1;

    uint8_t tx[3] = { (uint8_t)(0x06 | ((ch & 0x04) >> 2)),
                       (uint8_t)((ch & 0x03) << 6),
                       0x00 };
    uint8_t rx[3] = {0};
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed_hz,
        .bits_per_word = 8
    };
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) return -1;

    return ((rx[1] & 0x0F) << 8) | rx[2];
}

void initialize_joy(int x, int y){
    if (x<min_x) min_x=x;
    if (y<min_y) min_y=y;
    if (x>max_x) max_x=x;
    if (y>max_y) max_y=y;
}

void calibrate_joy (int fd) {
    for (int i =0; i < 50; i++){
        int x = joy_read_ch(fd, 0, SPI_SPEED);
        int y = joy_read_ch(fd,1, SPI_SPEED);

    initialize_joy(x,y);
    sleep_ms(10);
    }
}

joystick_direction_t joy_get_direction(int x, int y){

int center_x = (max_x+min_x)/2;
int center_y = (max_y+min_y)/2;

int thresh_x = (max_x-min_x)*0.45;
int thresh_y = (max_y-min_y)*0.45;

    if (thresh_x < 100) thresh_x=150;
    if (thresh_y < 100 ) thresh_y=150;
    if (x>center_x + thresh_x){ 
        return DIR_RIGHT;
    }
    if (x<center_x + thresh_x){
        return DIR_LEFT;
    }
    if(y > center_y + thresh_y){
        return DIR_UP;
    }
    if(y < center_y + thresh_y){ 
        return DIR_DOWN;
    }    else{
        return DIR_NONE;
    }
}