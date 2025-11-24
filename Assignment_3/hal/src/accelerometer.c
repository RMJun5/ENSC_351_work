#include "hal/accelerometer.h"
#include "beatbox.h"
#include "audioMixer.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
// Threshold for detecting a hit
#define XTHRESH 400
#define YTHRESH 400
#define ZTHRESH 400
// Hysteresis (re-arm threshold smaller than trigger threshold)
#define REARM_FACTOR 2                 // re-arm when |delta| < THRESH/REARM_FACTOR

// SPI channel assignments for each axis
static int channelX = 2; // X-axis channel
static int channelY = 3; // Y-axis channel
static int channelZ = 5; // Z-axis channel

// Baseline values for each axis
static int16_t baselineX = 0;
static int16_t baselineY = 0;
static int16_t baselineZ = 0;

//Armed flags for each axis
static bool armedX = false;
static bool armedY = false;
static bool armedZ = false;

static bool accInitialized = false;
static int adc_fd = -1; // SPI file descriptor
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
    if (value > 4095) {
        value = 4095;
    }
    if (value < baselineX) {
        value = baselineX;
    }
    return (int16_t)value;
}
// Read the Y-axis data from the accelerometer
int16_t accelerometer_read_y(void) {
      if (!accInitialized || adc_fd < 0) {
        fprintf(stderr, "accelerometer_read_y: not initialized\n");
        return 0;
    }
    int value = read_adc_ch(adc_fd, channelY, speed);
    if (value < 0) { 
        fprintf(stderr, "accelerometer_read_y: read error\n");
        return 0;
    }
    if (value > 4095) {
        value = 4095;
    }
    if (value < baselineY) {
        value = baselineY;
    }
    return (int16_t)value;
}
// Read the Z-axis data from the accelerometer
int16_t accelerometer_read_z(void) {
      if (!accInitialized || adc_fd < 0) {
        fprintf(stderr, "accelerometer_read_z: not initialized\n");
        return 0;
    }
    int value = read_adc_ch(adc_fd, channelZ, speed);
    if (value < 0) { 
        fprintf(stderr, "accelerometer_read_z: read error\n");
        return 0;
    }
    if (value > 4095) {
        value = 4095;
    }
    if (value < baselineZ) {
        value = baselineZ;
    }
    return (int16_t)value;
}
//Generate a sound based on the accelerometer data
void accelerometer_generate_sound(int16_t x, int16_t y, int16_t z) {
  int16_t dX = x - baselineX;
   int16_t dY = y - baselineY;
   int16_t dZ = z - baselineZ;

    int16_t ax = (int16_t)(abs(dX));
    int16_t ay = (int16_t)(abs(dY));
    int16_t az = (int16_t)(abs(dZ));

    // X-axis: snare
    // Y-axis: hi-hat
    // Z-axis: bass drum

    // X-axis/snare
    if (armedX && (ax > XTHRESH)) {
        wavedata_t *snare = NULL;
        if (beatbox.type == ROCK) snare = &beatbox.data.rock.snare;
        else if (beatbox.type == CUSTOM) snare = &beatbox.data.custom.snare;
        if (snare && snare->pData) AudioMixer_queueSound(snare);
        armedX = false;
    } else if (!armedX && (ax < (XTHRESH / REARM_FACTOR))) {
        armedX = true;
    }
    // Y-axis/hihat
    if (armedY && (ay > YTHRESH)) {
        wavedata_t *hihat = NULL;
        if (beatbox.type == ROCK) hihat = &beatbox.data.rock.hihat;
        else if (beatbox.type == CUSTOM) hihat = &beatbox.data.custom.hihat;
        if (hihat && hihat->pData) AudioMixer_queueSound(hihat);
        armedY = false;
    } else if (!armedY && (ay < (YTHRESH / REARM_FACTOR))) {
        armedY = true;
    }
    // Z-axis/kick/bass
    if (armedZ && (az > ZTHRESH)) {
        wavedata_t *kick = NULL;
        if (beatbox.type == ROCK) kick = &beatbox.data.rock.kick;
        else if (beatbox.type == CUSTOM) kick = &beatbox.data.custom.kick;
        if (kick && kick->pData) AudioMixer_queueSound(kick);
        armedZ = false;
    } else if (!armedZ && (az < (ZTHRESH / REARM_FACTOR))) {
        armedZ = true;
    }

    /*Logic:
    *  - allows one hit per big swing (axis must drop below smaller threshold to re-arm)
    *  - still lets fast shaking generate multiple hits (100+ BPM is fine)
    */
}

void accelerometer_cleanup(void) {
    // Cleanup accelerometer resources
    if (!accInitialized) {
        return;
    }
    if (adc_fd >= 0) {
        close(adc_fd);
        adc_fd = -1;
    }
    accInitialized = false;
}