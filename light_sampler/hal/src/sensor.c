#include "hal/sensor.h"
#include "hal/timing.h"
#include <stdio.h>
#include <stdlib.h>

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

int fd = open(DEV_PATH, O_RDWR);

int CH0; // light sensor

const char* dev;
uint8_t mode;
uint8_t bits;
uint32_t speed;

// thread variables
// thread sum variable
// bool thread_running = false;
static atomic_bool thread_running = false;
//size of readings
long long size = 0;
// thread limit variable (unsure if what limit should be)
long long limit = 10000000; // 10 million
//Thread ID
static pthread_t sampler_tid;
//Thread attributes
pthread_attr_t attr;
// Thread function to generate sum of readings 
void* sensor_size_runner (void* arg) 
{
    if (!thread_running) {
        thread_running = true;
    }
    long long *limitptr = (long long*) arg;
    long long limit = *limitptr;

    long long size = 0;
    for (long long i = 0; i < limit; i++) {
        size += i;
    }
    //What to do with sum?

}
long long thread_read_size() {

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    //Create thread to compute sum
    pthread_create(&sampler_tid, &attr, sensor_size_runner, (void*)&limit);
    //Wait for thread to finish
    pthread_join(sampler_tid, NULL);
    //Sum of readings from 0 to limit-1
    printf("Sum from 0 to %lld = %lld\n", limit-1, size);
    return size;
}
void thread_cleanup(void* arg) {
     //Cleanup   
    pthread_attr_destroy(&attr);
    pthread_cancel(sampler_tid);
    pthread_exit(NULL);
}

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
    if (fd < 0) { perror("opendd"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_MODE, &DEV_MODE) == -1) { perror("mode"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &DEV_BITS) == -1) { perror("bpw"); return 1;}
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &DEV_SPEED) == -1) { perror("speed"); return 1;}

    // read channel 0 (connected to light sensor)
    CH0 = read_adc_ch(fd, 0, DEV_SPEED);
    printf("CH0=%d\n", CH0);
    return CH0;
    sensor_cleanup();
}
void sensor_cleanup() {
    // Clean up device
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}
int getHistorySize() {
    // read from history.txt and return the number of lines
    FILE* fileID = fopen(HISTORY_FILE, "r");
    if (fileID == NULL) {
        perror("Error opening file");
        return -1;
    }
    long long t_size = thread_read_size();
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
    size = thread_read_size();
    fclose(fileID);
    if (size == 0) return 0.0;
    double average = (double)total / size;
    return average;
}

