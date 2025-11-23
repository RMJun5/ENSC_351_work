#include "periodTimer.h"
#include "audioMixer.h"
#include <stdio.h>
#include <unistd.h>
// #include "beatbox.h"


wavedata_t beatSound;

int main(void) {
    // BeatBoxInit();

    AudioMixer_init();

    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100054__menegass__gui-drum-ch.wav", &beatSound);

    AudioMixer_queueSound(&beatSound);

    while (1) {
        // main loop
        sleep(1);
    }


    AudioMixer_freeWaveFileData(&beatSound);
    AudioMixer_cleanup();
    // BeatBoxCleanup();

    return 0;
}
