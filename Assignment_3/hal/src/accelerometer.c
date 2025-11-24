#include "hal/accelerometer.h"
#include "beatbox.h"
#include "audioMixer.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>


static bool accInitialized = false;
static int channelX = 6; // X-axis channel
static int channelY = 7; // Y-axis channel
static int channelZ = 8; // Z-axis channel
static const char* dev = "/dev/spidev0.0"; // SPI device path
static uint8_t mode = 0; // SPI mode 0
static uint8_t bits = 8; // bits per word
static uint32_t speed = 250000; // clock speed

/* Use the global BeatBox instance from the application so we can queue
 * individual samples when an axis triggers. The instance is defined in
 * `app/src/main.c` as `BeatBox beatbox;`.
 */
extern BeatBox beatbox;

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
    // Initialize the accelerometer hardware
    if (accInitialized) {
        printf("accelerometer: already initialized\n");
        return;
    }

    adc_fd = open(dev, O_RDWR);
    if (adc_fd < 0) {
        perror("accelerometer: open SPI device");
        adc_fd = -1;
        return;
    }

    if (ioctl(adc_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("accelerometer: SPI_IOC_WR_MODE");
        close(adc_fd);
        adc_fd = -1;
        return;
    }
    if (ioctl(adc_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("accelerometer: SPI_IOC_WR_BITS_PER_WORD");
        close(adc_fd);
        adc_fd = -1;
        return;
    }

    if (ioctl(adc_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("accelerometer: SPI_IOC_WR_MAX_SPEED_HZ");
        close(adc_fd);
        adc_fd = -1;
        return;
    }
    
    // Calibrate baseline (board at rest, consistent orientation)
    const int samples = 100;
    long sumX = 0, sumY = 0, sumZ = 0;
    int valid = 0;
    for (int i = 0; i < samples; i++) {
        int vx = read_adc_ch(adc_fd, channelX, speed);
        int vy = read_adc_ch(adc_fd, channelY, speed);
        int vz = read_adc_ch(adc_fd, channelZ, speed);
        if (vx < 0 || vy < 0 || vz < 0) {
            continue;
        }
        valid++;
        sumX += vx;
        sumY += vy;
        sumZ += vz;
        usleep(2000); // 2ms between calibration samples
    }
    if (valid > 0) {
        baselineX = (int16_t)(sumX / valid);
        baselineY = (int16_t)(sumY / valid);
        baselineZ = (int16_t)(sumZ / valid);
    } else {
        baselineX = baselineY = baselineZ = 0;
    }

    armedX = true;
    armedY = true;
    armedZ = true;
    printf("Accelerometer initialized. Baseline X=%d Y=%d Z=%d\n",
           baselineX, baselineY, baselineZ);
    accInitialized = true;

}


// Read the X-axis data from the accelerometer
int16_t accelerometer_read_x(void) {
      if (!accInitialized || adc_fd < 0) {
        fprintf(stderr, "accelerometer_read_x: not initialized\n");
        return 0;
    }
    int value = read_adc_ch(adc_fd, channelX, speed);
    if (value < 0) { 
        fprintf(stderr, "accelerometer_read_x: read error\n");
        return 0;
    }
    int fd = open(dev, O_RDWR);
    int value = read_adc_ch(fd, channelX, speed);
    close(fd);
    return value;
}
// Read the Y-axis data from the accelerometer
int16_t accelerometer_read_y(void) {
    if (!accInitialized) {
        printf("Error: Accelerometer not initialized.\n");
        return 0;
    }
    int fd = open(dev, O_RDWR);
    int value = read_adc_ch(fd, channelY, speed);
    close(fd);
    return value;
}

// read the Z-axis data from the accelerometer
int16_t accelerometer_read_z(void) {
    if (!accInitialized) {
        printf("Error: Accelerometer not initialized.\n");
        return 0;
    }
    int fd = open(dev, O_RDWR);
    int value = read_adc_ch(fd, channelZ, speed);
    close(fd);
    return value;
}


int Accelerometer_read(float *ax, float *ay, float *az) {
    *ax = accelerometer_read_x();
    *ay = accelerometer_read_y();
    *az = accelerometer_read_z();
    return 0;
}