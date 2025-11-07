
// #include "hal/sampler.h"
// #include "hal/led.h"
// #include "hal/periodTimer.h"
// #include "hal/help.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <errno.h>
// #include <string.h>
// #include <pthread.h>
// #include <stdatomic.h>
// #include <time.h>

#define _GNU_SOURCE
#include "hal/sampler.h"
#include "hal/help.h"       // devRead(), read_adc_ch(), ADCtoV(), sleep_ms(), Period_*()
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <errno.h>

/*
 * Lightweight sampling module (internal names changed for clarity):
 * - keeps a rolling samples of the most recent second of ADC samples
 * - archives the just-completed samples into a history snapshot
 * - maintains an exponential moving average of readings
 */

// Thread control
// static pthread_t samplerThread_id;

static atomic_bool running = false;       // Thread running flag
static int spi_fd = -1;                   // SPI file descriptor
static int adc_channel = 0;               // ADC channel
static const double alpha = 0.999;        // EMA smoothing: 99.9% weight on previous
static const int SAMPLE_INTERVAL_MS = 1;  // 1ms between samples


// Main sampler state
static Sampler samp = {
    .curr = {NULL, 0},                    // Current second's samples
    .hist = {NULL, 0},                   // Previous second's history and dip count
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
    
    spi_fd = devRead(DEV_PATH, DEV_MODE, DEV_BITS, DEV_SPEED);
    if (spi_fd < 0) {
        perror("sampler_init(): SPI open failed");
        exit(EXIT_FAILURE);
    }

    /* Signal the worker thread to run */
    // atomic_store_explicit(&running, true, memory_order_release);
    // Period_init();
    // pthread_attr_t attr;
    // pthread_attr_init(&attr);

    samp.curr.samples = malloc(sizeof(double) * MAX_SAMPLESPERSECOND);
    if (!samp.curr.samples) {
        perror("sampler_init(): malloc failed");
        exit(-1);
    }

    samp.curr.size = 0;
    samp.hist.samples = NULL;
    samp.hist.size = 0;
    samp.stats.avg = 0.0;
    samp.stats.totalSamplesTaken = 0;

    // } else if (read_adc_ch(fd, adc_channel, DEV_SPEED) < 0) {
    //     perror("sampler_init(): failed to read adc channel");
    //     exit(-1);
    // }
    Period_init();
    atomic_store(&running, true);
    samp.initialized = true;

    // // pthread_create(&samplerThread_id, &attr, samplerThread, NULL);
    // pthread_create(@samp.threadID, NULL, samplerThread, NULL);
    // // pthread_attr_destroy(&attr);
    // // free(samp.curr.samples);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&samp.threadID, &attr, samplerThread, NULL);
    pthread_attr_destroy(&attr);
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
    // atomic_store_explicit(&running, false, memory_order_release);
    // pthread_join(samplerThread_id, NULL);

    atomic_store(&running, false);
    pthread_join(samp.threadID, NULL);

    pthread_mutex_lock(&samp.lock);
    free(samp.curr.samples);
    free(samp.hist.samples);
    samp.curr.samples = NULL;
    samp.hist.samples = NULL;
    samp.curr.size = 0;
    samp.hist.size = 0;
    pthread_mutex_unlock(&samp.lock);

    Period_cleanup();
    samp.initialized = false;

    /* Free buffers under the sampler lock */
//     pthread_mutex_lock(&samp.lock);
//     free(samp.curr.samples);
//     free(samp.hist.samples);
//     samp.curr.samples = samp.hist.samples = NULL;
//     samp.curr.size = samp.hist.size = 0;
//     pthread_mutex_unlock(&samp.lock);

//     Period_cleanup();
//     samp.initialized = false;
}


/**
 * @brief Return how many samples are present in the preserved history snapshot.
 * 
 * @return the number of samples in the history
 */ 
int sampler_getHistorySize() {
    pthread_mutex_lock(&samp.lock);
    int n = samp.hist.size;
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
    int n = samp.hist.size;
    if (size) *size = n;
    if (n > 0 && samp.hist.samples) {
        histcopy = malloc(sizeof(double) * (size_t)n);
        if (histcopy) {
            memcpy(histcopy, samp.hist.samples, sizeof(double) * (size_t)n);
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
long long sampler_getNumsamplesTaken(){
    // printf("Total number of samples: %d", samp.stats.total);
    // return samp.stats.total;
    pthread_mutex_lock(&samp.lock);
    long long t = samp.stats.totalSamplesTaken;
    pthread_mutex_unlock(&samp.lock);
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

    double adcVal = read_adc_ch(fd, adc_channel, DEV_SPEED);

    // while(samp.initialized) {
    //     double adcVal = read_adc_ch(adc_channel, DEV_SPEED, DEV_BITS); // int read_adc_ch(int fd, int ch, uint32_t speed_hz)
            if (adcVal < 0) {
                fprintf(stderr,"sampler_getCurrentReading(): ADC read faileds\n returning error value -1 ");
                sleep_ms(100);
                return -1;
            }
        if (adcVal > MAX_ADCVALUE){
            adcVal = MAX_ADCVALUE;
        } 
        return adcVal;
        // pthread_mutex_lock(&samp.lock);
        // if (curr.size < MAX_SAMPLE_SIZE){
        //    curr.samples[curr.size] = adcVal;
        //    curr.size ++;
        // } else {
        //     printf("sampler_getCurrentReading(): buffer full/sample size too big
        //             \n returning error value -1");
        //     sampler_cleanup();
        //     return -1;
        // }
        // pthread_mutex_unlock(& lock);
        // return adcVal;
    //}
}

/**
 * @brief Move the current data to the history
 * 
 */
void sampler_moveCurrentDataToHistory() {

    pthread_mutex_lock(&samp.lock);

    free(samp.hist.samples);
    samp.hist.samples = NULL;
    samp.hist.size = 0;

     if (samp.curr.size > 0 && samp.curr.samples) {
        double *dst = malloc(sizeof(double) * (size_t)samp.curr.size);
        if (dst) {
            memcpy(dst, samp.curr.samples, sizeof(double) * (size_t)samp.curr.size);
            samp.hist.samples = dst;
            samp.hist.size    = samp.curr.size;
        }
        // start a fresh second
        samp.curr.size = 0;
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
    pthread_mutex_lock (&samp.lock);
    if (samp.stats.totalSamplesTaken == 1) {
        samp.stats.avg = adc;
    } else {
        samp.stats.avg = alpha * samp.stats.avg + (1.0 - alpha) * adc;
    }
    double avg = samp.stats.avg;
    pthread_mutex_unlock(&samp.lock);
    return avg;
}

/**
 * @brief Get the number of dips
 * 
 * @return the number of dips
 */
int sampler_getHistDips(){

    pthread_mutex_lock(&samp.lock);
    // double *src = samp.hist.samples;
    int n = samp.hist.size; 

    if (n <= 0 || samp.hist.samples == NULL) {
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

    memcpy(hist, samp.hist.samples, sizeof(double)*(size_t)n);
    pthread_mutex_unlock(&samp.lock);

    double currEma = samp.stats.avg;
    //pthread_mutex_unlock(&samp.lock);
    int dips = 0;
    dip_detected = false;

    DIP_EVENTS state = ARMED;

    for (int i = 0; i < n; i++){
        // const double samples= hist[i];
        // currEma = sampler_getAverageReading(samples);
        currEma = sampler_getAverageReading(hist[i]);
        // if (samples<= currEma-DIP_TRIG){
        if (!dip_detected && hist[i] <= currEma - DIP_TRIG){
            dip_detected = true;
            state = DIPPING;
            dips++;
        } else if (dip_detected && hist[i] >= currEma-DIP_REARM){
            dip_detected = false;
            state = ARMED;
        }
    }
    free(hist);
    //samp.hist.dips = dips; // maybe not needed
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
    
    int fd = spi_fd;
    if (fd < 0) {
        perror("open ADC device");
        return NULL;
    }

    // Track time for 1-second intervals
    // struct timespec last_rotation = {0, 0};
    struct timespec last_rotation;
    clock_gettime(CLOCK_MONOTONIC, &last_rotation);
    
    // while (atomic_load_explicit(&running, memory_order_acquire)) {
    while (atomic_load(&running)) {

        // Direct ADC read (don't use getCurrentReading to avoid double-locking)
        double adcVal = read_adc_ch(fd, adc_channel, DEV_SPEED);
        if (adcVal < 0) {
            fprintf(stderr, "sampler: ADC read failed\n");
            sleep_ms(SAMPLE_INTERVAL_MS);
            continue;
        }
        
        // Clamp to valid range
        if (adcVal > MAX_ADCVALUE) {
            adcVal = MAX_ADCVALUE;
        }
        
        // Get current time for rotation check
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        
        pthread_mutex_lock(&samp.lock);
        
        // Check if we need to rotate buffers (1 second elapsed)
        if ((now.tv_sec - last_rotation.tv_sec) == 1) {
            sampler_moveCurrentDataToHistory();
            last_rotation = now;
        }
        
        // Update running statistics
        samp.stats.totalSamplesTaken++;
        if (samp.stats.totalSamplesTaken == 1)
            samp.stats.avg = adcVal; // first sample
        else
            samp.stats.avg = alpha * samp.stats.avg + (1.0 - alpha) * adcVal; // EMA update
        
        // Store in current buffer if there's room
        if (samp.curr.size < MAX_SAMPLESPERSECOND) {
            samp.curr.samples[samp.curr.size++] = adcVal;
        }
        
        // Mark event for timing analysis
        // Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        
        pthread_mutex_unlock(&samp.lock);
        
        // Mark event for timing analysis
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);

        // Sleep precisely 1ms (sampling interval)
        sleep_ms(SAMPLE_INTERVAL_MS);
    }
    return NULL;
}