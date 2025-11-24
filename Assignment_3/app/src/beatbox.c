#include "beatbox.h"
#include <stdlib.h>
#include <stdio.h>

void BeatBox_playSample(BeatBox *beatbox, const int sample) {

    switch (sample) {
        case 0:
            printf("Kick\n");
            AudioMixer_queueSound(&beatbox->data.rock.kick);
            break;
        case 1:
            printf("Snare\n");
            AudioMixer_queueSound(&beatbox->data.rock.snare);
            break;
        case 2:
            printf("HiHat\n");
            AudioMixer_queueSound(&beatbox->data.rock.hihat);
            break;
    }
}

void BeatBox_playRock(BeatBox *beatbox, int bpm) {


    int step_us = (60 * 1000000) / (bpm * 4);    // 4 steps (16th notes) per beat

        AudioMixer_queueSound(&beatbox->data.rock.kick);
        AudioMixer_queueSound(&beatbox->data.rock.hihat);
        usleep(step_us);
        AudioMixer_queueSound(&beatbox->data.rock.hihat);
        usleep(step_us);
        AudioMixer_queueSound(&beatbox->data.rock.snare);
        AudioMixer_queueSound(&beatbox->data.rock.hihat);
        usleep(step_us);
        AudioMixer_queueSound(&beatbox->data.rock.hihat);
        usleep(step_us);
}

void BeatBox_playCustom(BeatBox *beatbox, int bpm) {
     
        int step_us = (60 * 1000000) / (bpm * 8);    // 4 steps (16th notes) per beat

        AudioMixer_queueSound(&beatbox->data.rock.kick);
        AudioMixer_queueSound(&beatbox->data.rock.hihat);
        usleep(step_us);

        AudioMixer_queueSound(&beatbox->data.rock.hihat);
        usleep(step_us);

        AudioMixer_queueSound(&beatbox->data.rock.hihat);
        usleep(step_us);

        AudioMixer_queueSound(&beatbox->data.rock.snare);
        AudioMixer_queueSound(&beatbox->data.rock.hihat);
        usleep(step_us);

        AudioMixer_queueSound(&beatbox->data.rock.hihat);
        usleep(step_us);

        AudioMixer_queueSound(&beatbox->data.rock.hihat);
        usleep(step_us);
}

void BeatBox_init(BeatBox *beatbox) {
    if (!beatbox) return;

    // ROCK sounds
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100051__menegass__gui-drum-bd-hard.wav", &beatbox->data.rock.kick);
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100059__menegass__gui-drum-snare-soft.wav", &beatbox->data.rock.snare);
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100053__menegass__gui-drum-cc.wav", &beatbox->data.rock.hihat);
    scaleVolume(&beatbox->data.rock.hihat, 0.3);

    // CUSTOM sounds
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100051__menegass__gui-drum-bd-hard.wav", &beatbox->data.custom.kick);
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100059__menegass__gui-drum-snare-soft.wav", &beatbox->data.custom.snare);
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100053__menegass__gui-drum-cc.wav", &beatbox->data.custom.hihat);
    scaleVolume(&beatbox->data.custom.hihat, 0.3);

    beatbox->type = NONE;
}

void BeatBox_cleanup(BeatBox *beatbox) {
    if (!beatbox) return;

    AudioMixer_freeWaveFileData(&beatbox->data.rock.kick);
    AudioMixer_freeWaveFileData(&beatbox->data.rock.snare);
    AudioMixer_freeWaveFileData(&beatbox->data.rock.hihat);
    AudioMixer_freeWaveFileData(&beatbox->data.custom.kick);
    AudioMixer_freeWaveFileData(&beatbox->data.custom.snare);
    AudioMixer_freeWaveFileData(&beatbox->data.custom.hihat);
}


