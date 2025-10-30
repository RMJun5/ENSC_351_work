#include "hal/sensor.h"
#include <stdlib.h>

#define HISTORY_FILE "history.txt"
#define HISTORY_SIZE 0

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

int sensor_read() {
    dev = "/dev/spidev0.0";
    mode = 0;
    bits = 8;
    speed = 250000;

    int fd = open(dev, O_RDWR);

    if (fd < 0) { perror("opendd"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) { perror("mode"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) { perror("bpw"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) { perror("speed"); return 1;}

    CH0 = read_adc_ch(fd, 0, speed);

    printf("CH0=%d\n", CH0);
    return CH0;
    close(fd);
}

int getHistorySize() {
    int numberOfLines;
    int ch;

    FILE* fileID = fopen(HISTORY_FILE, "r");
    if (fileID == NULL) {
        perror("Error opening file");
        return -1;
    }
    
    while ((ch = fgetc(fileID)) != EOF) {
        if (ch == '\n') {numberOfLines++;}
    }
    fclose(fileID);
    return numberOfLines;
}

double* getHistory(int* size) {
    int t_size = getHistorySize

    // read from history.txt and return the number of lines
    FILE* fileID = fopen(HISTORY_FILE, "r");
    if (fileID == NULL) {
        perror("Error opening file");
        return -1;
    }
    
    double* history = (double*) malloc(t_size * sizeof(double));
    if (history == NULL) {
        perror("Error allocating memory for history");
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
    int size = getHistorySize();

    int total = 0;
    FILE* fileID = fopen(HISTORY_FILE, "r");
    if (fileID == NULL) {
        perror("Error opening file");
        return -1;
    }
    
    fclose(fileID);
    if (size == 0) return 0.0;
    double average = (double)total / size;
    return average;
}