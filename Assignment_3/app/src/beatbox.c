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

    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100051__menegass__gui-drum-bd-hard.wav", &beatbox->data.custom.kick);
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100059__menegass__gui-drum-snare-soft.wav", &beatbox->data.custom.snare);
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100053__menegass__gui-drum-cc.wav", &beatbox->data.custom.hihat);
    scaleVolume(&beatbox->data.rock.hihat, 0.3);

    // const char *customPaths[5] = {
    //     "/mnt/remote/myApps/wave-files/100051__menegass__gui-drum-bd-hard.wav",
    //     "/mnt/remote/myApps/wave-files/100059__menegass__gui-drum-snare-soft.wav",
    //     "/mnt/remote/myApps/wave-files/100053__menegass__gui-drum-cc.wav",
    //     "/mnt/remote/myApps/wave-files/100055__menegass__gui-drum-co.wav",
    //     "/mnt/remote/myApps/wave-files/100060__menegass__gui-drum-splash-hard.wav"
    // };

    // char path[256];
    // for (int i = 0; i < beatbox->data.custom.numSounds; i++) {
    //     snprintf(path, sizeof(path), "%s", customPaths[i]);
    //     AudioMixer_readWaveFileIntoMemory(path, &beatbox->data.custom.sounds[i]);
    // }
    // printf("num sounds: %d\n", beatbox->data.custom.numSounds);
    // // Default type
    
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

        // if (beatbox->data.custom.sounds) {
        // for (int i = 0; i < beatbox->data.custom.numSounds; i++) {
        //     AudioMixer_freeWaveFileData(&beatbox->data.custom.sounds[i]);
        // }
        // free(beatbox->data.custom.sounds);
        // beatbox->data.custom.sounds = NULL;
        // beatbox->data.custom.numSounds = 0;

    // beatbox->type = NONE;  // optional: reset type after cleanup
}


