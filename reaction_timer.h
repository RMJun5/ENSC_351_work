#ifndef REACTION_TIMER_H
#define REACTION_TIMER_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <linux/spi/spidev.h>

#ifndef RT_MAX_PATH
    #define RT_MAX_PATH 256
#endif


// timing using nanosleep
static inline void rt_sleep_ms(long ms){
    struct timespec ts;
    ts.tv_sec = ms/1000;
    ts.tv_nsec = (ms%1000)* 1000000L;
    nanosleep(&ts, NULL);
}

// writing a string to sysfile
static inline int rt_write_sysfs_(const char *path, const char *s){
    FILE *f = fopen(path, "w");
    if(!f) {perror(path); return -1;}
    if (fprintf(f, "%s", s) <=0) {perror("fprintf"); fclose(f); return -1;}
    if (fclose(f) !=0){perror("fclose"); return -1;}
    return 0;
}

// Build "/sys/class/leds/<name>/<leaf>" into buf
static inline int rt_build_led_path_(char *buf, size_t buflen,
  const char *name, const char *leaf) {
    if (!buf || !name || !leaf) return -1;
    int n = snprintf(buf, buflen, "/sys/class/leds/%s/%s", name, leaf);
    return (n < 0 || (size_t)n >= buflen) ? -1 : 0;
}

// --- Public API ---

// Set LED trigger, e.g., "none", "heartbeat", "timer", ...
static inline int rt_led_set_trigger(const char *led_name, const char *trigger) {
    char path[RT_MAX_PATH];
    if (rt_build_led_path_(path, sizeof(path), led_name, "trigger") != 0) return -1;
    return rt_write_sysfs_(path, trigger);
}

// Turn LED on/off (1 or 0) via brightness
static inline int rt_led_set_on(const char *led_name, int on) {
    char path[RT_MAX_PATH];
    if (rt_build_led_path_(path, sizeof(path), led_name, "brightness") != 0) return -1;
    return rt_write_sysfs_(path, on ? "1" : "0");
}

// Convenience: ensure manual control, then flash ON for `on_ms`, then OFF.
static inline int rt_led_flash_ms(const char *led_name, long on_ms) {
    if (rt_led_set_trigger(led_name, "none") != 0) return -1;
    if (rt_led_set_on(led_name, 1) != 0) return -1;
    rt_sleep_ms(on_ms);
    if (rt_led_set_on(led_name, 0) != 0) return -1;
    return 0;
}

static inline int rt_spi_opendevice(const char *dev, uint8_t mode, uint8_t bits, uint32_t speed_hz) {
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


#endif