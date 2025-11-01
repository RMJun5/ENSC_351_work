#include "hal/sampler.h"
#include "hal/led.h"

#include "periodTimer.h"
#include "hal/help.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

static pthread_t samplerID;
static double *currSamples = NULL;
static int currSize = 0;

static double *histSamples = NULL;
static int histSize = 0;

// Stats
static long long totSamples = 0;
static double avgExp = 0.0;
static bool firstSample = true;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

Period_statistics_t stats = {0};
static bool samplerInitialized = false;
static bool go = false;

//read light sensor raw adc values
void sampler_init() {
    if (samplerInitialized) {
        fprintf(stderr, "Sensor already initialized\n");
        return;
    }   
    samplerInitialized = true;
    go =true;
    Period_init();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    currSamples = malloc(sizeof(double)* MAX_SAMPLESPERSECOND);
    if (!currSamples){
        stderr()
    }
    pthread_create(&samplerID, &attr);
}

void sampler_cleanup() {
    // Clean up device
     if (!samplerInitialized) {
        fprintf(stderr, "LED not initialized\n");
        return;
    }
    samplerInitialized = false;
    pthread_mutex_lock(&lock);
    free(currSamples);
    free(histSamples);
    currSamples= histSamples =NULL;
    currSize = histSize = 0;
    pthread_mutex_unlock(&lock);

    pthread_exit(0);
}

void sampler_moveCurrentDataToHistory(){

}

int sampler_getHistorySize() {
    // read from history.txt and return the number of lines
    getPeriodStatisticsAndClear(NUM_PERIOD_EVENTS, &stats);
    int size = stats.numSamples;
    return size;
}

double* sampler_getHistory(int* size) {
    double* history = (double*) malloc(size * sizeof(double));
    getPeriodStatisticsAndClear(,&stats);

    return history;
}

// Get the average light level (not tied to the history)
double sampler_getAverageReading (){
    Period_getStatisticsAndClear(NUM_PERIOD_EVENTS, &stats);
    int size = stats.numSamples;
    if (size == 0) return 0.0;
    double average = (double)total / size;
    return average;
}

void* samplerThread(void* arg){
    long long *limitPTR = (long long*) arg;
    long long limit = *limitPTR;
    for (long long i = 0; i<=limit; i++){
        
    }
}