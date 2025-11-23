#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "periodTimer.h"
#include "audioMixer.h"
#include "beatbox.h"
#include "hal/encoder.h"

// read json file:
// #include <json-c/json.h>
// #include "beatbox.h"

Rotation prev_input = STOPPED;
Rotation user_input;


int main(void) {
    AudioMixer_init();
    encoder_init();

    RockBeat rockBeat;
    BeatType type = BeatBox_init(1, &rockBeat);

    // main loop
    while (1) {
       // BeatType type = BeatBox.init("ROCK");  // get beattype from user input in node js server: json request
       // open json files data:

        user_input = read_encoder();

        //Get the user input
        while (1) {
            user_input = read_encoder();
            
            // For stability, check for consistency between previous and current inputs - stable if the same
            if (user_input != STOPPED && user_input == prev_input) {
                printf("Stable input: %d\n", user_input);
                user_input = prev_input;
                break;
            }
            prev_input = user_input;
            sleep(0.01);
        }

        if (user_input == CW) {
            type = ROCK;
        } else if (user_input == CCW) {
            type = ROCK;
        }

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

        //usleep(500); // small delay to prevent CPU spin
    }


    AudioMixer_freeWaveFileData(&rockBeat.snare);
    AudioMixer_freeWaveFileData(&rockBeat.kick);
    AudioMixer_freeWaveFileData(&rockBeat.hihat);
    AudioMixer_cleanup();
    // BeatBoxCleanup();

    return 0;
}

