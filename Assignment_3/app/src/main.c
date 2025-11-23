#include "periodTimer.h"
#include "audioMixer.h"
#include "beatbox.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// read json file:
// #include <json-c/json.h>
// #include "beatbox.h"



int main(void) {
    AudioMixer_init();

    RockBeat rockBeat;
    BeatType type = BeatBox_init(1, &rockBeat);

    // main loop
    while (1) {
       // BeatType type = BeatBox.init("ROCK");  // get beattype from user input in node js server: json request
       // open json files data:

        switch(type) {
            case ROCK:
                BeatBox_play(&rockBeat);
                break;
            case CUSTOM:
                BeatBox_custom();
                break;
            case NONE:
                break;
            default:
                break;
        }
    }


    AudioMixer_freeWaveFileData(&rockBeat.snare);
    AudioMixer_freeWaveFileData(&rockBeat.kick);
    AudioMixer_freeWaveFileData(&rockBeat.hihat);
    AudioMixer_cleanup();
    // BeatBoxCleanup();

    return 0;
}

