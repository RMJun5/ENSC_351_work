#include "periodTimer.h"
#include "audioMixer.h"
#include <stdio.h>
// #include "beatbox.h"


wavedata_t beatSound;

int main(void) {
    // BeatBoxInit();

    AudioMixer_init();

    AudioMixer_readWaveFileIntoMemory("./audio/wave-files/100059__menegass__Gui_DRUM_SNARE_soft.wav", &beatSound);

    AudioMixer_queueSound(&beatSound);

    // while (1) {
    //     // main loop

    // }


    AudioMixer_freeWaveFileData(&beatSound);
    AudioMixer_cleanup();
    // BeatBoxCleanup();

    return 0;
}
