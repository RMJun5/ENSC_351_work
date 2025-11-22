#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include "beatbox.h"

void BeatBoxInit(void)
{
    BeatBoxSetVolume(BEATBOX_VOLUME_DEFAULT);
    // Initialize any data structures you need here.
    // For example, you might want to initialize an array of sound bites
    // to indicate that no sound bites are currently being played.
    
}