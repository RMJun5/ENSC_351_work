
#include "hal/sampler.h"
#include "hal/led.h"
#include "periodTimer.h"
#include "hal/help.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

/*
 * Lightweight sampling module (internal names changed for clarity):
 * - keeps a rolling samples of the most recent second of ADC samples
 * - archives the just-completed samples into a history snapshot
 * - maintains an exponential moving average of readings
 */

static int adc_channel = 0;
static Sampler samp = {
    .curr = {NULL, 0},
    .hist = {NULL, 0,0},
    .stats = {0.0, 0},
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .initialized = false
};
static double alpha = 0.999;            // smoothing factor for EMA 

//State bool
static bool dip_detected = false; 

// Initialize sampling system and start the worker thread.
void sampler_init() {
    if (samp.initialized) {
        fprintf(stderr, "sampler: already initialized\n");
        return;
    }

    samp.initialized = true;

    samp.curr.samples = malloc(sizeof(double) * MAX_samplesPERSECOND);
    if (!samp.curr.samples) {
        perror("sampler_init(): malloc failed");
        exit(-1);
    } else if (read_adc_ch(adc_channel) < 0) {
        perror("sampler_init(): failed to read adc channel");
        exit(-1);
    }
}


// Tear down sampling and free sampless. 
void sampler_cleanup() {
    if (!samp.initialized) {
        fprintf(stderr, "sampler: not initialized\n");
        return;
    }

    samp.initialized = false;
}

void sampler_moveCurrentDataToHistory(){
    pthread_mutex_lock(&samp.lock);
    free(samp.hist.samples);
    samp.hist.samples = malloc(sizeof(double)* samp.curr.size);
    if (samp.hist.samples){
        memcpy(samp.hist.samples,samp.curr.samples,sizeof(double)*samp.curr.size);
        samp.hist.size = samp.curr.size;
        samp.curr.size = 0;
    } else{
        samp.hist.size = 0;
    }
    pthread_mutex_unlock(&samp.lock);
}


// Return how many samples are present in the preserved history snapshot. 
int sampler_getHistorysize() {
    pthread_mutex_lock(&samp.lock);
    int size = samp.hist.size;
    pthread_mutex_unlock(&samp.lock);
    return size;
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
double* sampler_getHistory(int size ) {
    double* histTemp = NULL;
    pthread_mutex_lock(&samp.lock);
    int n = samp.hist.size;
    if (size) *size = n;
    if (n > 0){
        histTemp = malloc(sizeof(double)*(size_t)n);
        if (histTemp){
            memcpy(histTemp,samp.hist.samples, sizeof(double)*(size_t)n); 
        }
    }
    pthread_mutex_unlock(&samp.lock);
    return histTemp;
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
        if (samp.curr.size < MAX_SAMPLE_SIZE){
           samp.curr.samples[samp.curr.size] = adcVal;
           samp.curr.size ++;
        } else {
            printf("sampler_getCurrentReading(): buffer full/sample size too big
                    \n returning error value -1");
            sampler_cleanup();
            return -1;
        }
        pthread_mutex_unlock(& samp.lock);
        return adcVal;
    }
}

/* Exponential moving average for the ADC samples. */
double sampler_getAverageReading(double adc) {
    pthread_mutex_lock (& samp.lock);
    if (samp.stats.total==1) {
        samp.stats.avg = adc;
    } else {
        samp.stats.avg = alpha * samp.stats.avg + (1.0 - alpha) * adc;
    }
    pthread_mutex_unlock(& samp.lock);
    return samp.stats.avg;
}
int sampler_getHistDips(){
    pthread_mutex_lock(&samp.lock);
    double *src = samp.hist.samples;
    int n = samp.hist.size; 
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
    samp.hist.dips = dips;
    return dips;
}


/* Worker thread: repeatedly sample ADC, update samples and stats. 
void* samplerThread(void* arg) {
    (void)arg;
    while (samp.initialized) {
        double adcVal = sampler_getCurrentReading();
        // Mark timing and then store sample under lock 
        pthread_mutex_lock(&samp.lock);
        samp.stats.total++;
        double avg = samp.stats.avg;
        avg =sampler_getAverageReading(adcVal);
        
        if (adcVal>avg)    
    
        printf("Total number of samples: %d", samp.stats.total);
        pthread_mutex_unlock(&samp.lock);

        sleep_ms(1000);

    }
    sampler_cleanup();
    return NULL;
}*/