#include "hal/accelerometer.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <stdint.h>

static bool accInitialized = false;
static int channelX = 2; // X-axis channel
static int channelY = 3; // Y-axis channel
static int channelZ = 5; // Z-axis channel
static const char* dev = "/dev/spidev0.0"; // SPI device path
static uint8_t mode = 0; // SPI mode 0
static uint8_t bits = 8; // bits per word
static uint32_t speed = 250000; // clock speed

int read_adc_ch(int fd, int ch, uint32_t speed_hz) {
    uint8_t tx[3] = {   (uint8_t)(0x006 | ((ch & 0x04) >> 2)),
                        (uint8_t)((ch & 0x03)<< 6),
                        0x00
                    };


    uint8_t rx[3] = {0};

    struct spi_ioc_transfer tr =
    {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed_hz,
        .bits_per_word = 8,
        .cs_change = 0
    };

    if(ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("SPI_IOC_MESSAGE");
        return -1;
    }

    return ((rx[1] & 0x0F) << 8) | rx[2];
}
// Initialize the accelerometer
void accelerometer_init(void) {
    // Initialize the accelerometer hardware (stub implementation)
    srand((unsigned int)time(NULL)); // Seed for random data generation
    printf("Accelerometer initialized.\n");
    accInitialized = true;
}
// Read the X-axis data from the accelerometer
int16_t accelerometer_read_x(void) {
    if (!accInitialized) {
        printf("Error: Accelerometer not initialized.\n");
        return 0;
    }
    read_adc_ch(open(dev, O_RDWR), channelX, speed);
    
    
}
// Read the Y-axis data from the accelerometer