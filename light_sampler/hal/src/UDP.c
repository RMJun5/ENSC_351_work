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
    .running = false,
    .socket = -1,
    .shutdown=NULL,
}

static char last_cmd[64]={0};

void send_text(int socket, UDP   )

void* udp_listener(void* arg){
    
}