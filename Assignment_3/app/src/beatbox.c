#include "beatbox.h"
#include <stdlib.h>
#include <stdio.h>

// void BeatBox_play(RockBeat *beat, int bpm) {
//     int step_us = (60 * 1000000) / (bpm * 4);    // 4 steps (16th notes) per beat

//     AudioMixer_queueSound(&beat->kick);
//     AudioMixer_queueSound(&beat->hihat);
//     usleep(step_us);                         // New function will make this wait depending on the BPM 
//     AudioMixer_queueSound(&beat->hihat);
//     usleep(step_us);
//     AudioMixer_queueSound(&beat->snare);
//     AudioMixer_queueSound(&beat->hihat);
//     usleep(step_us);
//     AudioMixer_queueSound(&beat->hihat);
//     usleep(step_us);
// }

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
    // for (int i = 0; i < beatbox->data.custom.numSounds; i++) {
            //     AudioMixer_queueSound(&beatbox->data.custom.sounds[i]);
            //     usleep(step_us);
            // }
            // break;
}

// BeatType BeatBox_init(int beat, BeatBox *beatbox) {
//     switch (beat) {
//         case 1:
//             AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100051__menegass__gui-drum-bd-hard.wav", &beatbox->data->rock->kick);
//             AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100059__menegass__gui-drum-snare-soft.wav", &beatbox->data->rock->snare);
//             AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100053__menegass__gui-drum-cc.wav", &beatbox->data->rock->hihat);
//             scaleVolume(&data->hihat, 0.3);    // scaled down cause the hihat is much louder
//             return ROCK;
//         case 2:
//             CustomBeat.numSounds = 5; // for example
//             CustomBeat.sounds = malloc(sizeof(wavedata_t) * customBeat.numSounds);

//             AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100051__menegass__gui-drum-bd-hard.wav", &customBeat.sounds[0]);
//             AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100059__menegass__gui-drum-snare-soft.wav", &customBeat.sounds[1]);
//             AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100053__menegass__gui-drum-cc.wav", &customBeat.sounds[2]);
//             AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100055__menegass__gui-drum-co.wav", &customBeat.sounds[3]);
//             AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100060__menegass__gui-drum-splash-hard.wav", &customBeat.sounds[4]);
//             return CUSTOM;
//         case 0:
//             return NONE;
//     }
//     return NONE;
// }

void BeatBox_init(BeatBox *beatbox) {
    if (!beatbox) return;

    // ROCK sounds
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100051__menegass__gui-drum-bd-hard.wav", &beatbox->data.rock.kick);
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100059__menegass__gui-drum-snare-soft.wav", &beatbox->data.rock.snare);
    AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100053__menegass__gui-drum-cc.wav", &beatbox->data.rock.hihat);
    scaleVolume(&beatbox->data.rock.hihat, 0.3);

    const char *customPaths[5] = {
        "/mnt/remote/myApps/wave-files/100051__menegass__gui-drum-bd-hard.wav",
        "/mnt/remote/myApps/wave-files/100059__menegass__gui-drum-snare-soft.wav",
        "/mnt/remote/myApps/wave-files/100053__menegass__gui-drum-cc.wav",
        "/mnt/remote/myApps/wave-files/100055__menegass__gui-drum-co.wav",
        "/mnt/remote/myApps/wave-files/100060__menegass__gui-drum-splash-hard.wav"
    };

    char path[256];
    for (int i = 0; i < beatbox->data.custom.numSounds; i++) {
        snprintf(path, sizeof(path), "%s", customPaths[i]);
        AudioMixer_readWaveFileIntoMemory(path, &beatbox->data.custom.sounds[i]);
    }

    // Default type
    beatbox->type = ROCK;
}

void BeatBox_cleanup(BeatBox *beatbox) {
    if (!beatbox) return;

        AudioMixer_freeWaveFileData(&beatbox->data.rock.kick);
        AudioMixer_freeWaveFileData(&beatbox->data.rock.snare);
        AudioMixer_freeWaveFileData(&beatbox->data.rock.hihat);

        if (beatbox->data.custom.sounds) {
        for (int i = 0; i < beatbox->data.custom.numSounds; i++) {
            AudioMixer_freeWaveFileData(&beatbox->data.custom.sounds[i]);
        }
        free(beatbox->data.custom.sounds);
        beatbox->data.custom.sounds = NULL;
        beatbox->data.custom.numSounds = 0;
        }
    // beatbox->type = NONE;  // optional: reset type after cleanup
}


