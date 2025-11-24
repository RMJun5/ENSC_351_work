#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "periodTimer.h"
#include "audioMixer.h"
#include "beatbox.h"
#include "hal/encoder.h"
#include <pthread.h>
#include "hal/joystick.h"
#include "hal/accelerometer.h"

#include <arpa/inet.h>

#define PORT 12345
#define BUF_SIZE 1024


BeatBox beatbox;
int bpm = 100;
int vol = 50;


// ACTION CALLBACKS FOR ENCODER AND JOYSTICK

void encoderAction(Rotation input, int *bpm, BeatBox *beatbox) {
    if (input == CW) *bpm += 5;
    else if (input == CCW) *bpm -= 5;
    else if (input == PRESSED) {
        if (beatbox->type == ROCK) beatbox->type = CUSTOM;
        else if (beatbox->type == CUSTOM) beatbox->type = ROCK;
        else if (beatbox->type == NONE) beatbox->type = ROCK;

        printf("Switched beat type to %d\n", beatbox->type);
    }
}


void joystickAction(Direction input, int *vol) {
    if (input == JS_UP) *vol += 5;
    else if (input == JS_DOWN) *vol -= 5;

    // Clamp volume
    if (*vol < 0) *vol = 0;
    if (*vol > 100) *vol = 100;

    AudioMixer_setVolume(*vol);
    printf("Volume: %d\n", *vol);
}




// THREADS:

void* drum_thread(void* arg) {
    while (1) {
        switch (beatbox.type) {
            case ROCK:
                BeatBox_playRock(&beatbox, bpm);
                break;
            case CUSTOM:
                BeatBox_playCustom(&beatbox, bpm);
                break;
            case NONE:
                break;
        }
        usleep(1000);
    }
    return NULL;
}

void* encoder_thread(void* arg) {
    Rotation prev_input = STOPPED;
    Rotation user_input;
    while (1) {
        user_input = read_encoder();
        if (user_input != STOPPED && user_input != prev_input) {
            encoderAction(user_input, &bpm, &beatbox);
        }
        Period_markEvent(PERIOD_EVENT_SAMPLE_ACCEL);
        prev_input = user_input;
        usleep(10000);
    }
    return NULL;
}

void* joystick_thread(void* arg) {
    Direction prev_js = IDLE;
    Direction input_js;
    while (1) {
        input_js = joystick();
        if (input_js != IDLE && input_js != prev_js) {  // act whenever joystick is moved
            joystickAction(input_js, &vol);
        }
        prev_js = input_js;

        Period_markEvent(PERIOD_EVENT_SAMPLE_AUDIO);
        usleep(1000);
    }
    return NULL;
}


void* accelerSoundThread(void* arg) {
    (void)arg; // Unused parameter
    while (1) {
        int16_t x = accelerometer_read_x();
        int16_t y = accelerometer_read_y();
        int16_t z = accelerometer_read_z();

        accelerometer_generate_sound(x, y, z);

        usleep(5000); // 5 ms
    }
    return NULL;
}

void* udp_thread(void* arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in hostAddr;
    hostAddr.sin_family = AF_INET;
    hostAddr.sin_port = htons(PORT);
    hostAddr.sin_addr.s_addr = inet_addr("192.168.7.1");

    unsigned long lastPrint = time(NULL);

    while (1) {
        unsigned long now = time(NULL);
        if (now != lastPrint) {   // 1Hz
            lastPrint = now;

            Period_statistics_t statsAudio, statsAccel;
            Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_AUDIO, &statsAudio);
            Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_ACCEL, &statsAccel);

                char buffer[256];
                snprintf(buffer, sizeof(buffer),
                        "Audio[%.2f, %.2f], avg: %.2f; Accel[%.2f, %.2f], avg: %.2f",
                        statsAudio.minPeriodInMs,
                        statsAudio.maxPeriodInMs,
                        statsAudio.avgPeriodInMs,
                        statsAccel.minPeriodInMs,
                        statsAccel.maxPeriodInMs,
                        statsAccel.avgPeriodInMs);

                sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&hostAddr, sizeof(hostAddr));
        }

        usleep(1000); // small sleep
    }
}




int main(void) {
    // Encoder
    Rotation prev_input = STOPPED;
    Rotation user_input;

    // Joystick
    Direction input_js;
    Direction prev_js = IDLE;    

    // Beatbox
    beatbox.type = NONE;

    // // Network
    // unsigned long lastPrint = time(NULL);
    // int sock = socket(AF_INET, SOCK_DGRAM, 0);
    // struct sockaddr_in hostAddr;
    // hostAddr.sin_family = AF_INET;
    // hostAddr.sin_port = htons(12345);
    // hostAddr.sin_addr.s_addr = inet_addr("192.168.7.1");

    // Initialize hardware and period timer
    AudioMixer_init();
    BeatBox_init(&beatbox);
    encoder_init();
    accelerometer_init();
    Period_init();
        
    pthread_t drum, enc, js, acc;
    pthread_t udp;
    pthread_create(&udp, NULL, udp_thread, NULL);
    pthread_create(&drum, NULL, drum_thread, NULL);
    pthread_create(&enc, NULL, encoder_thread, NULL);
    pthread_create(&js, NULL, joystick_thread, NULL);
    pthread_create(&acc, NULL, accelerSoundThread, NULL);

    pthread_join(drum, NULL);
    pthread_join(enc, NULL);
    pthread_join(js, NULL);
    pthread_join(acc, NULL);
        

        // Period_markEvent(PERIOD_EVENT_SAMPLE_AUDIO);
        // Period_markEvent(PERIOD_EVENT_SAMPLE_ACCEL);

        // /*
        // Time between refilling audio playback buffer
        //     Each time your code finishes filling the playback buffer, mark the interval/event.
        //     Format: Audio[{min}, {max}] avg {avg}/{num-samples}
        // Time between samples of the accelerometer
        //     Each time your code reads the accelerometer, mark an interval/event.
        //     Format: Accel[{min}, {max}] avg {avg}/{num-samples}
        // */
        // unsigned long now = time(NULL);
        //     if (now != lastPrint) {       // 1Hz
        //         lastPrint = now;

        //         Period_statistics_t statsAudio, statsAccel;
        //         Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_AUDIO, &statsAudio);
        //         Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_ACCEL, &statsAccel);


        //         //printf("Sent stats: %s\n", buffer);
        //     }

        //     // small delay to control loop speed
        //    // usleep(10000);  // 10 ms loop
    


    BeatBox_cleanup(&beatbox);
    AudioMixer_cleanup();
    clean_encoder();
    accelerometer_cleanup();
    // BeatBoxCleanup();

    return 0;
}
