#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "hal/help.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <ctype.h>
#include <errno.h>


static double max_adc = 4095.0; 
static double vref = 3.3; 


void write2file(const char *path, const char *value) {
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        perror("open");
        return;
    }
    int n = fprintf(f, "%s", value);
    if (n <= 0) {
        perror("write");
    }
    if (fclose(f) != 0) {
        perror("close");
    }
}

int devRead(const char * path, uint8_t mode, uint8_t bits, uint32_t speed) {
    int fd = open(path, O_RDWR);
    if (fd < 0) { 
        perror("open");
        close(fd);
        return -1;
    }
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) { 
        perror("mode");
        close(fd);
        return -1;
    }
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("bpw");
        close(fd);
        return -1;
    }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("speed");
        close(fd);
        return -1;
    }
    return fd;
}

Period_statistics_t printStatistics(enum Period_whichEvent Event) {
    Period_statistics_t stats;
    Period_getStatisticsAndClear(Event, &stats);

    printf("Light sensor statistics:\n");
    printf("  Num samples: %d\n", stats.numSamples);
    printf("  Min period (ms): %.3f\n", stats.minPeriodInMs);
    printf("  Max period (ms): %.3f\n", stats.maxPeriodInMs);
    printf("  Avg period (ms): %.3f\n", stats.avgPeriodInMs);

    return stats;
}

int read_adc_ch(int fd, int ch, uint32_t speed_hz) {
    if (ch < 0 || ch > 7) {
        errno = EINVAL;
        return -1;
    }

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

double nanotoms (int ns){
    double ms = (double)ns / 1000000.0;
    return ms;
}


long long getTimeInMs(void) {
    struct timespec spec;

    // Fallback to real time if monotonic fails
    if (clock_gettime(CLOCK_MONOTONIC, &spec) != 0) {
        clock_gettime(CLOCK_REALTIME, &spec);
    }

    // clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = (long long)spec.tv_sec;
    long long nanoSeconds = (long long)spec.tv_nsec;
    long long milliSeconds = seconds * 1000LL + nanoSeconds / 1000000LL;
    return milliSeconds;
}

double ADCtoV(int reading){
    return (reading*vref)/max_adc;
}

void sleep_ms(long long ms){
    
    if (ms <= 0) return; // if the time is negative

    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    
    long long delayNs = ms * NS_PER_MS;
    int seconds = (int)(delayNs / NS_PER_SECOND);
    int nanoseconds = (int)(delayNs % NS_PER_SECOND);
    struct timespec reqDelay = { .tv_sec = seconds, .tv_nsec = nanoseconds };
    nanosleep(&reqDelay,(struct timespec *)NULL);
}

static void sanitize_line(char *s) {
    if (!s) return;
    
    // Trim leading/trailing whitespace; convert to lowercase for command compare
    // (Assignment: lower-case accepted; case-insensitive optional.)
    char *p = s;
    
    while (*p && (*p == '\r' || *p == '\n')) { 
        p++; // unlikely at front, but safe
    }

    // left trim spaces
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    if (p != s) memmove(s, p, strlen(p) + 1);
    
    // right trim
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) {
        s[--n] = '\0';
    }
}

static void to_lower_ascii(char *s){
    if (!s) return;
    for (; *s; ++s) {
        *s = (char)tolower((unsigned char)*s);
    }
}