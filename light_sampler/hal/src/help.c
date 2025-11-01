#include "hal/help.h"
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
    if (fd < 0) { perror("open"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) { perror("mode"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) { perror("bpw"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) { perror("speed"); return 1;}
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

    if(ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) return -1;

    return ((rx[1] & 0x0F) << 8) | rx[2];
}