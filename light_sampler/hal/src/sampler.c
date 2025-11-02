
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

/*
 * Lightweight sampling module (internal names changed for clarity):
 * - keeps a rolling samples of the most recent second of ADC samples
 * - archives the just-completed samples into a history snapshot
 * - maintains an exponential moving average of readings
 */

static int adc_channel = 0;
static pthread_t thread_id;
static double *current_samples = NULL;   // newest samples (up to 1 second)
static int current_size = 0;

static double *history_samples = NULL;   // last archived samples 
static int history_size = 0;

static double alpha = 0.999;            // smoothing factor for EMA 
static long long total_samples = 0;
static bool first_sample = true;
static double AvgRead = 0.0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// State bools
static bool is_initialized = false;
static atomic_bool sampling_enabled = false;
static bool dip_detected = false; 

// Initialize sampling system and start the worker thread.
void sampler_init() {
    if (is_initialized) {
        fprintf(stderr, "sampler: already initialized\n");
        return;
    }

    is_initialized = true;
    sampling_enabled = true;

    Period_init();

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    current_samples = malloc(sizeof(double) * MAX_SAMPLESPERSECOND);
    if (!current_samples) {
        perror("sampler: malloc failed");
        exit(-1);
    } else if (read_adc_ch(adc_channel) < 0) {
        perror("sampler: failed to read adc channel");
        exit(-1);
    }

    //pass address of total_samples as the thread argument (keeps original behavior)
    pthread_create(&thread_id, &attr, samplerThread, &total_samples);
    pthread_attr_destroy(&attr);
}


// Tear down sampling and free sampless. 
void sampler_cleanup() {
    if (!is_initialized) {
        fprintf(stderr, "sampler: not initialized\n");
        return;
    }

    is_initialized = false;
    sampling_enabled = false;

    pthread_mutex_lock(&mutex);
    free(current_samples);
    free(history_samples);
    current_samples = history_samples = NULL;
    current_size = history_size = 0;
    pthread_mutex_unlock(&mutex);

    pthread_join(thread_id, NULL);
    // exit the calling thread context as original did; keep call to Period cleanup 
    pthread_exit(0);
    Period_cleanup();
}


// Move the current in-memory samples into the history snapshot.
void sampler_moveCurrentDataToHistory() {
    pthread_mutex_lock(&mutex);
    free(history_samples);
    history_samples = malloc(sizeof(double) * current_size);
    if (history_samples) {
        memcpy(history_samples, current_samples, sizeof(double) * current_size);
        history_size = current_size;
    } else {
        history_size = 0;
    }
    current_size = 0; // reset for next second 
    pthread_mutex_unlock(&mutex);
}


// Return how many samples are present in the preserved history snapshot. 
int sampler_getHistorySize() {
    pthread_mutex_lock(&mutex);
    int size = history_size;
    pthread_mutex_unlock(&mutex);
    return size;
}

void sampler_getTimingStatistics(Period_statistics_t *pStats) {
    if (pStats) {
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, pStats);
    }
}

/* Get/clear total samples statistics (preserve original mapping). */
void sampler_getTotalSamples(Period_statistics_t *pStats) {
    if (pStats) {
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_FINAL, pStats);
    }
}


// Return an allocated copy of the last archived history. Caller must free(). 
double* sampler_getHistory(int* size) {
    pthread_mutex_lock(&mutex);
    double* copy = NULL;
    if (history_size > 0) {
        copy = malloc(sizeof(double) * history_size);
        if (copy) {
            memcpy(copy, history_samples, sizeof(double) * history_size);
        }
    }
    if (size) {
        *size = history_size;
    }
    pthread_mutex_unlock(&mutex);
    return copy;
}


/* Exponential moving average for the ADC samples. */
double sampler_getAverageReading(double adc_val) {
    if (first_sample) {
        AvgRead = adc_val;
        first_sample = false;
    } else {
        AvgRead = alpha * AvgRead + (1.0 - alpha) * adc_val;
    }
    return AvgRead;
}


/* Worker thread: repeatedly sample ADC, update sampless and stats. */
void* samplerThread(void* arg) {
    long long *limit_ptr = (long long*) arg;
    long long limit = *limit_ptr;

    sampler_init();
    while (sampling_enabled) {
        double adc_val = read_adc_ch(adc_channel);
        if (adc_val < 0) {
            perror("samplerThread: ADC read failed");
            sleep_ms(1000); /* pause 1 second */
            continue;
        }

        /* Mark timing and then store sample under lock */
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        pthread_mutex_lock(&mutex);

        AvgRead =sampler_getAverageReading(adc_val);

        if (current_size < MAX_SAMPLESPERSECOND) {
            current_samples[current_size] = adc_val;
            current_size++;
        }

        (void)sampler_getHistorySize(); /* preserve original call site behavior */

        total_samples++;

        pthread_mutex_unlock(&mutex);
    }

    sampler_cleanup();
    return NULL;
}