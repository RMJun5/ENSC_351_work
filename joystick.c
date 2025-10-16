#define _POSIX_C_SOURCE 200809L
#include "joystick.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <errno.h>
#include <math.h>

struct joystick { 
    int fd;
    uint32_t speed_hz
    int up 
    int down
    int deadzone // percent 0 to 100
};

joystick_t *joy_open(const char *dev, uint32_t speed_hz, int ch) {
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
    return fd;  
}

joystick_t joy_read (int fd, int ch, uint32_t speed_hz){
    if (ch<0 || ch>7) return -1;
    uint8_t tx[3] = { (uint8_t)(0x06 | ((ch & 0x04) >> 2)), (uint8_t)((ch & 0x03) << 6), 0x00 };
    uint8_t rx[3] = {0};   
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed_hz,
        .bits_per_word = 8,
        .cs_change = 0
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) return -1;
    val =  ((rx[1] & 0x0F) << 8) | rx[2]; // 12-bit result
    return val;
}

