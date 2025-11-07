
#include "hal/sampler.h"
#include "hal/led.h"
#include "periodTimer.h"
#include "hal/help.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

/*
 * Lightweight sampling module (internal names changed for clarity):
 * - keeps a rolling samples of the most recent second of ADC samples
 * - archives the just-completed samples into a history snapshot
 * - maintains an exponential moving average of readings
 */
// Thread control
static pthread_t samplerThread_id;
static atomic_bool running = false;

static int adc_channel = 0;
static const int SAMPLE_INTERVAL_MS = 1;  // 1ms between samples

// Main sampler state
static Sampler samp = {
    .curr = {NULL, 0},                   // Current second's samples
    .hist = {NULL, 0, 0},                // Previous second's history and dip count
    .stats = {0.0, 0},                   // EMA and total samples
    .lock = PTHREAD_MUTEX_INITIALIZER,    // Protects all shared state
    .initialized = false                  // Initialization flag
};

static const double alpha = 0.999;        // EMA smoothing: 99.9% weight on previous
static bool dip_detected = false;         // Dip detection state

// Forward declaration of thread function
static void* samplerThread(void* arg);

// Initialize sampling system and start the worker thread.
void sampler_init() {
    if (samp.initialized) {
        fprintf(stderr, "sampler: already initialized\n");
        return;
    }

    samp.initialized = true;
    /* Signal the worker thread to run */
    atomic_store_explicit(&running, true, memory_order_release);

    Period_init();

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    double current_samples = malloc(sizeof(double) * MAX_SAMPLESPERSECOND);
    if (!current_samples) {
        perror("sampler_init(): malloc failed");
        exit(-1);
    } else if (read_adc_ch(adc_channel) < 0) {
        perror("sampler_init(): failed to read adc channel");
        exit(-1);
    }
    samp.curr.samples = current_samples;
    pthread_create(&samplerThread_id, &attr, samplerThread, NULL);
    pthread_attr_destroy(&attr);
    free(current_samples);
}


// Tear down sampling and free sampless. 
void sampler_cleanup() {
    if (!samp.initialized) {
        fprintf(stderr, "sampler: not initialized\n");
        return;
    }

    /* Stop the worker thread and wait for it to exit */
    atomic_store_explicit(&running, false, memory_order_release);
    pthread_join(samplerThread_id, NULL);

    /* Free buffers under the sampler lock */
    pthread_mutex_lock(&samp.lock);
    free(samp.curr.samples);
    free(samp.hist.samples);
    samp.curr.samples = samp.hist.samples = NULL;
    samp.curr.size = samp.hist.size = 0;
    pthread_mutex_unlock(&samp.lock);

    Period_cleanup();
    samp.initialized = false;
}

void sampler_moveCurrentDataToHistory(){
        pthread_mutex_lock(&samp.lock);
    free(hist.samples);
    double history = malloc(sizeof(double)* curr.size);
    if (history){
        memcpy(hist.samples,curr.samples,sizeof(double)*curr.size);
        samp.hist.samples = history;
        samp.hist.size = samp.curr.size;
        samp.curr.size = 0;
    } else{
        samp.hist.size = 0;
    }
    free(history);
    pthread_mutex_unlock(&samp.lock);
}


// Return how many samples are present in the preserved history snapshot. 
int sampler_getHistorySize() {
    pthread_mutex_lock(&samp.lock);
    int n = samp.hist.size;
    pthread_mutex_unlock(&samp.lock);
    return n;
}

void sampler_getTimingStatistics(Period_statistics_t *pStats) {
    if (pStats) {
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, pStats);
    }
}

/* Get/clear total samples statistics*/
void sampler_getTotalsamples(Period_statistics_t *pStats) {
    if (pStats) {
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_FINAL, pStats);
    }
}


// Return an allocated copy of the last archived history. Caller must free(). 
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
long long sampler_getNumsamplesTaken(){
    printf("Total number of samples: %d", samp.stats.total);
    return samp.stats.total;
}

double sampler_getCurrentReading(){
    while(samp.initialized){
        double adcVal = read_adc_ch(adc_channel);
            if (adcVal < 0) {
                fprintf(stderr,"sampler_getCurrentReading(): ADC read failed
                            \n returning error value -1 ");
                sleep_ms(100);
                return -1;
            }
        if (adcVal > MAX_ADCVALUE){
            adcVal = MAX_ADCVALUE;
        } 
        pthread_mutex_lock(&samp.lock);
        if (curr.size < MAX_SAMPLE_SIZE){
           curr.samples[curr.size] = adcVal;
           curr.size ++;
        } else {
            printf("sampler_getCurrentReading(): buffer full/sample size too big
                    \n returning error value -1");
            sampler_cleanup();
            return -1;
        }
        pthread_mutex_unlock(& lock);
        return adcVal;
    }
}
void sampler_moveCurrentDataToHistory(){
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
/* Exponential moving average for the ADC samples. */
double sampler_getAverageReading(double adc) {
    pthread_mutex_lock (& lock);
    if (samp.stats.total==1) {
        samp.stats.avg = adc;
    } else {
        samp.stats.avg = alpha * samp.stats.avg + (1.0 - alpha) * adc;
    }
    pthread_mutex_unlock(& lock);
    return samp.stats.avg;
}
int sampler_getHistDips(){
    pthread_mutex_lock(&samp.lock);
    double *src = hist.samples;
    int n = hist.size; 
    if (n<=0||src == NULL){
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
    memccpy(hist, src, sizeof(double)*(size_t)n);
    double currEma = samp.stats.avg;
    pthread_mutex_unlock(&samp.lock);
    
    int dips = 0;
    enum DIP_EVENTS state = ARMED;
    dip_detected = false;
    for (int i=0; i<n;i++){
        const double samples= hist[i];
        currEma = sampler_getAverageReading(samples);

        if (samples<= currEma-DIP_TRIG){
            dip_detected = true;
            state = DIPPING;
            ++dips;
        } else if (samples>= currEma-DIP_REARM){
            dip_detected = false;
            state = ARMED;
        }
    }
    free(hist);
    hist.dips = dips;
    return dips;
}


// Worker thread: repeatedly sample ADC, update buffers and stats
void* samplerThread(void* arg) {
    (void)arg;  // unused
    
    // Track time for 1-second intervals
    struct timespec last_rotation = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &last_rotation);
    
    while (atomic_load_explicit(&running, memory_order_acquire)) {
        // Direct ADC read (don't use getCurrentReading to avoid double-locking)
        double adcVal = read_adc_ch(adc_channel);
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
        samp.stats.total++;
        samp.stats.avg = (samp.stats.total == 1) ? 
            adcVal : // first sample
            alpha * samp.stats.avg + (1.0 - alpha) * adcVal; // EMA update
        
        // Store in current buffer if there's room
        if (curr.size < MAX_SAMPLESPERSECOND) {
            curr.samples[curr.size++] = adcVal;
        }
        
        // Mark event for timing analysis
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        
        pthread_mutex_unlock(&samp.lock);
        
        // Sleep precisely 1ms (sampling interval)
        sleep_ms(SAMPLE_INTERVAL_MS);
    }
    return NULL;
}