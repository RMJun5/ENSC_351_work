#define _POSIX_C_SOURCE 200809L
#include "hal/UDP.h"
#include "hal/sampler.h"
#include "periodTimer.h"
#include "hal/help.h"


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

static UDP udp = {
    .running = false;
}

void* udp_listener(void* arg){

}