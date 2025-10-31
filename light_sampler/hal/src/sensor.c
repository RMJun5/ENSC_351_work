#include "hal/sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

//configurable parameters
#define SAMPLE_MS            1                       // 1 ms between samples
#define SAMPLES_PER_SECOND   (1000 / SAMPLE_MS)     // 1000
#define HISTORY_CAPACITY     SAMPLES_PER_SECOND     // keep last 1 second of samples
#define EXP_ALPHA            0.999                   // weight previous average at 99.9%

#define HISTORY_FILE "history.txt"
#define HISTORY_SIZE 0
// Device parameters
#define DEV_PATH "/dev/spidev0.0"
#define DEV_MODE 0
#define DEV_BITS 8
#define DEV_SPEED 250000

Period_statistics_t stats = {0};
int CH0; // light sensor

const char* dev;
uint8_t mode;
uint8_t bits;
uint32_t speed;

static int read_adc_ch(int fd, int ch, uint32_t speed_hz) {

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

//read light sensor raw adc values
int sensor_read() {
    int fd = open(DEV_PATH, O_RDWR);
     uint8_t mode = DEV_MODE;
    uint8_t bits = DEV_BITS;
    uint32_t speed_hz = DEV_SPEED;
    if (fd < 0) { perror("open"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) { perror("mode"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) { perror("bpw"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) { perror("speed"); return 1;}
    Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);
    printf("Light sensor statistics:\n");
    printf("  Num samples: %d\n", stats.numSamples);
    printf("  Min period (ms): %.3f\n", stats.minPeriodInMs);
    printf("  Max period (ms): %.3f\n", stats.maxPeriodInMs);
    printf("  Avg period (ms): %.3f\n", stats.avgPeriodInMs);
    // read channel 0 (connected to light sensor)
    CH0 = read_adc_ch(fd, 0, DEV_SPEED);
    printf("CH0=%d\n", CH0);
    return CH0;
    sensor_cleanup(); 
}
void sensor_cleanup() {
    // Clean up device
    int fd = open(DEV_PATH, O_RDWR);
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    Period_cleanup();
}

void moveCurrentDataToHistory(){
    fileID = fopen(HISTORY_FILE, "a");
    if (fileID == NULL) {
        perror("Error opening file for appending");
        return;
    }
    fprintf(fileID, "%d\n", CH0);
    fclose(fileID);
}

int getHistorySize() {
    // read from history.txt and return the number of lines
    FILE* fileID = fopen(HISTORY_FILE, "r");
    if (fileID == NULL) {
        perror("Error opening file");
        return -1;
    }
    getPeriodStatisticsAndClear(NUM_PERIOD_EVENTS, &stats);
    long long t_size = (long long) stats.numSamples;
    fclose(fileID);
    return (int)t_size;
}

double* getHistory(int* size) {
    FILE* fileID = fopen(HISTORY_FILE, "r");
    if (fileID == NULL) {
        perror("Error opening file");
        return NULL;
    }
    // get size;
    long long t_size = thread_read_size();
    double* history = (double*) malloc(t_size * sizeof(double));
    if (history == NULL) {
        perror("Error allocating memory for history");
        fclose(fileID);
        return NULL;
    }
    char line[256];
    int index = 0;
    while (fgets(line, sizeof(line), fileID)) {
        history[index] = atof(line);
        index++;
    }
    fclose(fileID);
    *size = index;
    return history;
}
// Get the average light level (not tied to the history)
double getAverageReading (){
    int total = 0;
    FILE* fileID = fopen(HISTORY_FILE, "r");
    if (fileID == NULL) {
        perror("Error opening file");
        return -1;
    }
    Period_getStatisticsAndClear(NUM_PERIOD_EVENTS, &stats);
    int size = stats.numSamples;
    fclose(fileID);
    if (size == 0) return 0.0;
    double average = (double)total / size;
    return average;
}

