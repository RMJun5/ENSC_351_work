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

static int samplerCH = 0;
static pthread_t samplerTID;
static double *currSamples = NULL;
static int currSize = 0;

static double *histSamples = NULL;
static int histSize = 0;

// Stats
static double Alpha = 0.999;
static long long totSamples = 0;
static bool firstSample = true;
static double Avg = 0.0;

static pthread_mutex_t latch = PTHREAD_MUTEX_INITIALIZER;

static bool samplerInitialized = false;
static atomic_bool goNextSample = false;



//read light sensor raw adc values
void sampler_init() {
    if (samplerInitialized) {
        fprintf(stderr, "Sensor already initialized\n");
        return;
    }   
    samplerInitialized = true;
    goNextSample =true;
    Period_init();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    currSamples = malloc(sizeof(double)* MAX_SAMPLESPERSECOND);
    if (!currSamples){
        perror("sampler_init:malloc")
        exit(-1);
    }else if (read_adc_ch(samplerCH)<0){
        perror("sampler_init:failed to read adc values")
        exit(-1)
    }
    pthread_create(&samplerTID, &attr, samplerThread, &totSamples);
    pthread_attr_destroy(&attr);
}

void sampler_cleanup() {
    // Clean up device
     if (!samplerInitialized) {
        fprintf(stderr, "LED not initialized\n");
        return;
    }
    samplerInitialized = false;
    goNextSample = false;
    pthread_mutex_lock(&latch);
    free(currSamples);
    free(histSamples);
    currSamples= histSamples =NULL;
    currSize = histSize = 0;
    pthread_mutex_unlock(&latch);
    
    pthread_join(samplerTID,NULL)
    pthread_exit(0);
    Period_cleanup();
}

void sampler_moveCurrentDataToHistory(){
    pthread_mutex_lock(&latch);
    free(histSamples);
    histSamples = malloc(sizeof(double)* currSize);
    if (histSamples){
        memcpy(histSamples,currSamples,sizeof(double)*currSize);
        histSize = currSize;
        currSize = 0;
    } else{
        histSize = 0;
    }
    pthread_mutex_unlock(&latch);
}

int sampler_getHistorySize() {
    pthread_mutex_lock(&latch);
    int size = histSize;
    pthread_mutex_unlock(&latch);
    return size;
}
void sampler_getTimingStatistics(Period_statistics_t *pStats) {
    if (pStats) {
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, pStats);
    }
}
// Get the total number of samples taken
void sampler_getTotalSamples(Period_statistics_t *pStats) {
    if (pStats) {
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_FINAL, pStats);
    }
}


double* sampler_getHistory(int* size) {
    pthread_mutex_lock(&latch);
    double* history = NULL;
    if(histSize>0){
        double* history = malloc(sizeof(double) * histSize);
        if(history){
            memcpy(history, histSamples, sizeof(double)*histSize);
        }
    }
    pthread_mutex_unlock(&latch);
    return history;
}


// Get the average light level (not tied to the history)
double sampler_getAverageReading (double adcVals){
    if(firstSample){
            Avg = adcVals;
            firstSample=false;
        } else {
            Avg= Alpha* Avg + (1.0-Alpha)*adcVals;
        }
    return Avg;
}

void* samplerThread(void* arg){
    long long *limitPTR = (long long*) arg;
    long long limit = *limitPTR;
 
    
    while(goNextSample){
        double adcVals = read_adc_ch(samplerCH)
            if (adcVals < 0) {
                perror("samplerThread:ADC Values reading failed");
                sleep_ms(1000) // sleep 1 second
                continue;
            }
        //Start sampling
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        pthread_mutex_lock(&latch);

        Avg = sampler_getAverageReading(adcVals);
            
         if (currSize < MAX_SAMPLESPERSECOND) {
            currSamples[currSize] = adcVals;
            currSize++;
        }
        int size = sampler_getHistorySize();
        
        totSamples++;
        
        pthread_mutex_unlock(&latch);

    }
}