#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "periodTimer.h"
#include "audioMixer.h"
#include "beatbox.h"
#include "hal/encoder.h"
#include "hal/joystick.h"


// read json file:
// #include <json-c/json.h>
// #include "beatbox.h"


int main(void) {
    // Encoder
    Rotation prev_input = STOPPED;
    Rotation user_input;

    // Joystick
    Direction input_js;
    Direction prev_js = IDLE;    

    RockBeat rockBeat;
    BeatType type;
    int bpm = 120;
    int vol = audioMixer_getVolume();


    AudioMixer_init();
    encoder_init();

    
    type = BeatBox_init(1, &rockBeat);

    // main loop
    while (1) {
       // BeatType type = BeatBox.init("ROCK");  // get beattype from user input in node js server: json request
       // open json files data:

        // user_input = read_encoder();

        //Get the user input
        while (1) {
            user_input = read_encoder();
            input_js = joystick();

            // For stability, check for consistency between previous and current inputs - stable if the same
            if ((user_input != STOPPED && user_input == prev_input) || (input_js != IDLE && input_js == prev_js)) {
                printf("Stable encoder input: %d\n", user_input);
                user_input = prev_input;
                break;
            }
            prev_js = input_js;
            prev_input = user_input;
            sleep(0.01);
        }

        // while (js_input == JS_UP) {
        //     vol += 5;
        // }
        // while (js_input == JS_DOWN) {
        //     vol -= 5;
        // }

        // handle encoder inputs
        if (user_input == CW) {
            // increase BPM
            bpm += 5;
        } else if (user_input == CCW) {
            bpm -= 5;
        } else if (user_input == PRESSED) {
            // iterate througjh beat types
            if (type == ROCK) {
                type = CUSTOM;
            } else if (type == CUSTOM) {
                type = ROCK;
            }
        }

        switch(type) {
            case ROCK:
                BeatBox_play(&rockBeat, bpm);
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
    clean_encoder();
    // BeatBoxCleanup();

    return 0;
}

