#include "beatbox.h"

void BeatBox_custom() {
    return;
}

void BeatBox_play(RockBeat *beat) {
    AudioMixer_queueSound(&beat->kick);
    AudioMixer_queueSound(&beat->hihat);
    usleep(250000);
    AudioMixer_queueSound(&beat->hihat);
    usleep(250000);
    AudioMixer_queueSound(&beat->snare);
    AudioMixer_queueSound(&beat->hihat);
    usleep(250000);
    AudioMixer_queueSound(&beat->hihat);
    usleep(250000);
}

BeatType BeatBox_init(int beat, RockBeat *data) {
    switch (beat) {
        case 1:
            AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100051__menegass__gui-drum-bd-hard.wav", &data->kick);
            AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100059__menegass__gui-drum-snare-soft.wav", &data->snare);
            AudioMixer_readWaveFileIntoMemory("/mnt/remote/myApps/wave-files/100053__menegass__gui-drum-cc.wav", &data->hihat);
            scaleVolume(&data->hihat, 0.3);
            return ROCK;
        case 2:
            //CustomBeat.numSounds = 5; // for example
            // customBeat.sounds = malloc(sizeof(wavedata_t) * customBeat.numSounds);
            // AudioMixer_readWaveFileIntoMemory("/path/kick.wav", &customBeat.sounds[0]);
            // AudioMixer_readWaveFileIntoMemory("/path/snare.wav", &customBeat.sounds[1]);
            // AudioMixer_readWaveFileIntoMemory("/path/hihat.wav", &customBeat.sounds[2]);
            // AudioMixer_readWaveFileIntoMemory("/path/tom.wav", &customBeat.sounds[3]);
            // AudioMixer_readWaveFileIntoMemory("/path/clap.wav", &customBeat.sounds[4]);
            return CUSTOM;
        case 0:
            return NONE;
    }
    return NONE;
}


