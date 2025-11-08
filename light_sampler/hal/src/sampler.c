#define _GNU_SOURCE
#include "hal/sampler.h"
#include "hal/help.h"       // devRead(), read_adc_ch(), ADCtoV(), sleep_ms(), Period_*()
#include "hal/periodTimer.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <errno.h>

/*
 * Lightweight sampling module (internal names changed for clarity):
 * - keeps a rolling samples of the most recent second of ADC samples
 * - archives the just-completed samples into a history snapshot
 * - maintains an exponential moving average of readings
 */

// Thread control
 static pthread_t samplerThread_id;

static atomic_bool running = false;       // Thread running flag
static int spi_fd = -1;                   // SPI file descriptor
static int adc_channel = 0;               // ADC channel
static const double alpha = 0.999;        // EMA smoothing: 99.9% weight on previous
static const int SAMPLE_INTERVAL_MS = 1;  // 1ms between samples


// Main sampler state
static Sampler samp = {
    .buffer = {NULL, 0},                    // Current second's samples
    .history = {NULL, 0, 0},                   // Previous second's history and dip count
    .stats = {0.0, 0},                      // EMA and total samples
    .lock = PTHREAD_MUTEX_INITIALIZER,      // Protects all shared state
    .initialized = false                    // Initialization flag
};


static bool dip_detected = false;         // Dip detection state

// Forward declaration of thread function
// static void* samplerThread(void* arg);
// static void sampler_moveCurrentDataToHistory(void);


/**
 * @brief Initialize sampling system and start the worker thread.
 * 
 */
void sampler_init() {
    if (samp.initialized) {
        fprintf(stderr, "sampler: already initialized\n");
        return;
    }  
    
    spi_fd = open(DEV_PATH, O_RDWR);
    if (spi_fd < 0) {
        perror("sampler_init(): SPI open failed");
        exit(EXIT_FAILURE);
    }
    
    samp.buffer.samples = malloc(sizeof(double) * MAX_SAMPLESPERSECOND);
    if (!samp.buffer.samples) {
        perror("sampler_init(): malloc failed");
        exit(-1);
    }

    samp.buffer.size = 0;
    samp.history.samples = NULL;
    samp.history.size = 0;
    samp.stats.avg = 0.0;
    samp.stats.totalSamplesTaken = 0;
     
    if (read_adc_ch(spi_fd, adc_channel, DEV_SPEED) < 0) {
         perror("sampler_init(): failed to read adc channel");
         exit(-1);
    }
    atomic_store(&running, true);
    pthread_attr_t sampAttr;
    pthread_attr_init(&sampAttr);
    int rc = pthread_create(&samp.threadID, &sampAttr, samplerThread, NULL);
     if (rc != 0) {
        perror("sampler_init(): pthread_create failed");
        samp.initialized = false;
        atomic_store(&running, false);
        exit(-1);
    }
    pthread_attr_destroy(&sampAttr);
    samp.initialized = true;
}

/**
 * @brief Tear down sampling and free sampless.
 * 
 */
void sampler_cleanup() {

    if (!samp.initialized) {
        fprintf(stderr, "sampler: not initialized\n");
        return;
    }

    /* Stop the worker thread and wait for it to exit */
    atomic_store(&running, false);
    pthread_join(samp.threadID, NULL);

    pthread_mutex_lock(&samp.lock);
    free(samp.buffer.samples);
    free(samp.history.samples);
    samp.buffer.samples = NULL;
    samp.history.samples = NULL;
    samp.buffer.size = 0;
    samp.history.size = 0;
    pthread_mutex_unlock(&samp.lock);

    samp.initialized = false;
}


/**
 * @brief Return how many samples are present in the preserved history snapshot.
 * 
 * @return the number of samples in the history
 */ 
int sampler_getHistorySize() {
    pthread_mutex_lock(&samp.lock);
    int n = samp.history.size;
    pthread_mutex_unlock(&samp.lock);
    return n;
}

/**
 * @brief Get/clear timing statistics
 * 
 * @param pStats the statistics to fill
 */
void sampler_getTimingStatistics(Period_statistics_t *pStats) {
    if (pStats) {
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, pStats);
    }
}

/**
 * @brief Get/clear total samples
 * 
 * @param pStats the struct to fill
 */
void sampler_getTotalsamples(Period_statistics_t *pStats) {
    if (pStats) {
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_FINAL, pStats);
    }
}

/**
 * @brief  Return an allocated copy of the last archived history. Caller must free(). 
 * 
 * @param size the number of samples in the history
 * @return a copy of the history
 */
double* sampler_getHistory(int*size ) {
    double* histcopy = NULL;

    pthread_mutex_lock(&samp.lock);
    int n = samp.history.size;
    if (size) *size = n;
    if (n > 0 && samp.history.samples) {
        histcopy = malloc(sizeof(double) * (size_t)n);
        if (histcopy) {
            memcpy(histcopy, samp.history.samples, sizeof(double) * (size_t)n);
        }
    }
    pthread_mutex_unlock(&samp.lock);

    return histcopy;
}

/**
 * @brief Get the number of samples taken
 * 
 * @return the number of samples taken
 */
long long sampler_getNumSamplesTaken(){
    pthread_mutex_lock(&samp.lock);
    long long t = samp.stats.totalSamplesTaken;
    pthread_mutex_unlock(&samp.lock);
    printf("Total number of samples: %d", samp.stats.totalSamplesTaken);
    return t;
}

/**
 * @brief gets the current reading from the ADC
 * 
 * @return the current reading 
 */
double sampler_getCurrentReading() {

    int fd = spi_fd;
    if (fd < 0) {
        perror("open ADC device");
        return -1.0;
    }

    pthread_mutex_lock(&samp.lock);

    if (samp.buffer.size >= MAX_SAMPLESPERSECOND){
        perror("buffer full/sample size too big \n returning error value -1");
        sampler_cleanup();
        return -1;
    }

    double adcVal = read_adc_ch(fd, adc_channel, DEV_SPEED); // int read_adc_ch(int fd, int ch, uint32_t speed_hz)
        
    if (adcVal < 0) {
        fprintf(stderr,"sampler_getCurrentReading(): ADC read faileds\n returning error value -1 ");
        sleep_ms(100);
        return -1;
    }
    if (adcVal > MAX_ADCVALUE){
        adcVal = MAX_ADCVALUE;
    } 
    
    samp.buffer.samples[samp.buffer.size] = adcVal;
    samp.buffer.size++;

    pthread_mutex_unlock(&samp.lock);

    return adcVal;
    
}

/**
 * @brief Move the current data to the history
 * 
 */
void sampler_moveCurrentDataToHistory() {

    pthread_mutex_lock(&samp.lock);

    free(samp.history.samples);
    samp.history.samples = NULL;
    samp.history.size = 0;

     if (samp.buffer.size > 0 && samp.buffer.samples) {
        double *dst = malloc(sizeof(double) * (size_t)samp.buffer.size);
        if (dst) {
            memcpy(dst, samp.buffer.samples, sizeof(double) * (size_t)samp.buffer.size);
            samp.history.samples = dst;
            samp.history.size    = samp.buffer.size;
        }
        // start a fresh second
        samp.buffer.size = 0;
    }

    pthread_mutex_unlock(&samp.lock);
}

/**
 * @brief Exponential moving average for the ADC samples. Thread Safe
 * 
 * @param adc the current ADC reading
 * @return the average
 * 
 */
double sampler_getAverageReading(double adc) {
    if (samp.stats.totalSamplesTaken == 1) {
        samp.stats.avg = adc;
    } else {
        samp.stats.avg = alpha * samp.stats.avg + (1.0 - alpha) * adc;
    }
    double avg = samp.stats.avg;
    return avg;
}

/**
 * @brief Get the number of dips
 * 
 * @return the number of dips
 */
int sampler_getHistDips(){

    pthread_mutex_lock(&samp.lock);
    int n = samp.history.size; 

    if (n <= 0 || samp.history.samples == NULL) {
        fprintf(stderr, "sampler_getHistDips(): History not found");
        pthread_mutex_unlock(&samp.lock);
        return 0;
    }

    double* hist = malloc(sizeof(double)*(size_t)n);

    if (!hist){
        fprintf(stderr, "sampler_getHistDips(): malloc error");
        pthread_mutex_unlock(&samp.lock);
        return 0;
    }

    memcpy(hist, samp.history.samples, sizeof(double)*(size_t)n);
    pthread_mutex_unlock(&samp.lock);

    double currEma = samp.stats.avg;
    int dips = 0;
    dip_detected = false;
    double latestSample = samp.buffer.samples[samp.buffer.size];

    DIP_EVENTS state = ARMED;

    for (int i = 0; i < n; i++){
        currEma = sampler_getAverageReading(hist[i]);
        
        if (!dip_detected && latestSample <= currEma - DIP_TRIG){
            dip_detected = true;
            state = DIPPING;
            dips++;
        } else if (dip_detected && latestSample >= currEma-DIP_REARM){
            dip_detected = false;
            state = ARMED;
        }
    }
    free(hist);
    return dips;
}

/**
 * @brief Worker thread: repeatedly sample ADC, update buffers and stats
 * 
 * @param arg 
 * @return void* 
 */
void* samplerThread(void* arg) {

    (void)arg;  // unused

    if (spi_fd < 0) {
        perror("open ADC device");
        return NULL;
    }

    // Track time for 1-second intervals
    // struct timespec last_rotation = {0, 0};
    struct timespec last_rotation;
    clock_gettime(CLOCK_MONOTONIC, &last_rotation);
    
    while (atomic_load(&running)) {

        // Direct ADC read (don't use getCurrentReading to avoid double-locking)
        int raw = read_adc_ch(spi_fd, adc_channel, DEV_SPEED);
        if (raw < 0) {
            fprintf(stderr, "sampler: ADC read failed\n");
            sleep_ms(SAMPLE_INTERVAL_MS);
            continue;
        }
        
        // Clamp to valid range
        if (raw > (int)MAX_ADCVALUE) {
            raw = (int)MAX_ADCVALUE;
        }
        double adcVal = (double)raw;

        pthread_mutex_lock(&samp.lock);
        // Update running statistics
        samp.stats.totalSamplesTaken++;
        if (samp.buffer.size < MAX_SAMPLESPERSECOND) {
            samp.buffer.samples[samp.buffer.size++] = adcVal;
        }
        double avg = sampler_getAverageReading(adcVal);

        pthread_mutex_unlock(&samp.lock);
        
        // Mark event for timing analysis
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
         // Track time for 1-second intervals
         // struct timespec last_rotation = {0, 0};
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
         time_t dsec = now.tv_sec - last_rotation.tv_sec;
        long   dnsec = now.tv_nsec - last_rotation.tv_nsec;
        if (dnsec < 0) { dsec -= 1; dnsec += 1000000000L; }

        if (dsec >= 1) {
            sampler_moveCurrentDataToHistory();   // locks internally
            Period_markEvent(PERIOD_EVENT_SAMPLE_FINAL);
            last_rotation = now;
        }
        // Sleep precisely 1ms (sampling interval)
        sleep_ms(SAMPLE_INTERVAL_MS);
    }
    return NULL;
}