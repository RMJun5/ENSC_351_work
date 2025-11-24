#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
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


#define POLL_MS 10      // 100 Hz
#define DEBOUNCE_MS 80      // refractory time per-axis (tunable)
#define AXIS_THRESHOLD 300    // threshold for delta (g-units or sensor units)
#define AXIS_RELEASE 120   // hysteresis release threshold (smaller than trigger)

// move to accelerometer module
typedef enum { 
    AXIS_X = 0, 
    AXIS_Y = 1, 
    AXIS_Z = 2 
} Axis;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

BeatBox beatbox;
int g_bpm = 100;
int g_volume = 80;
int g_mode = 0;


// ACTION CALLBACKS FOR ENCODER AND JOYSTICK

void encoderAction(Rotation input, BeatBox *beatbox) {
    pthread_mutex_lock(&lock);
    if (input == CW) g_bpm += 5;
    else if (input == CCW) g_bpm -= 5;
    else if (input == PRESSED) {
        if (beatbox->type == ROCK) beatbox->type = CUSTOM;
        else if (beatbox->type == CUSTOM) beatbox->type = NONE;
        else if (beatbox->type == NONE) beatbox->type = ROCK;

        printf("Switched beat type to %d\n", beatbox->type);
    }
    pthread_mutex_unlock(&lock);
}

void joystickAction(Direction input, Direction *prev_js) {
    pthread_mutex_lock(&lock);

    input = getFilteredJoystickInput();
    if (input != *prev_js) {
        if (input == JS_DOWN)  g_volume -= 5;
        if (input == JS_UP) g_volume += 5;

        AudioMixer_setVolume(g_volume);
        *prev_js = input;
    }

    if (g_volume < 0) g_volume = 0;
    if (g_volume > 100) g_volume = 100;

    pthread_mutex_unlock(&lock);
    printf("Volume: %d\n", g_volume);

}

static void play_axis_sound(Axis axis) {
    const int SAMPLE_KICK = 0; // z axis
    const int SAMPLE_SNARE = 1; // y axis
    const int SAMPLE_HIHAT = 2; // x axis

    if (axis == AXIS_Z) {
        BeatBox_playSample(&beatbox, SAMPLE_KICK);
        printf("[AIRDRUM] trigger Z (kick)\n");
    } else if (axis == AXIS_Y) {
        BeatBox_playSample(&beatbox, SAMPLE_SNARE);
        printf("[AIRDRUM] trigger Y (snare)\n");
    } else if (axis == AXIS_X) {
        BeatBox_playSample(&beatbox, SAMPLE_HIHAT);
        printf("[AIRDRUM] trigger X (hihat)\n");
    }
}

// helpers
static uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
}

static int parse_int_safe(const char* str, int* out) {
    if (!str || !out) return 0;

    // Skip leading spaces
    while (*str && isspace((unsigned char)*str)) str++;

    char* endptr;
    long val = strtol(str, &endptr, 10);

    // Check if conversion succeeded and consumed the whole number
    if (str == endptr) return 0;          // no digits found
    while (*endptr) {
        if (!isspace((unsigned char)*endptr)) return 0; // extra non-space chars
        endptr++;
    }

    *out = (int)val;
    return 1; // success
}

const char* beatTypeStr(int type) {
    switch(type) {
        case ROCK:   return "ROCK";
        case CUSTOM: return "CUSTOM";
        case NONE:   return "NONE";
        default:     return "UNKNOWN";
    }
}

static int clamp_mode(int m) {
    if (m < ROCK || m > NONE) return NONE;  
    return m;
}


// THREADS:

void* drum_thread(void* arg) {
    while (1) {
        switch (beatbox.type) {
            case ROCK:
                BeatBox_playRock(&beatbox, g_bpm);
                break;
            case CUSTOM:
                BeatBox_playCustom(&beatbox, g_bpm);
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
            encoderAction(user_input, &beatbox);
        }
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
            joystickAction(input_js, &prev_js);
        }
        prev_js = input_js;

        Period_markEvent(PERIOD_EVENT_SAMPLE_AUDIO);
        usleep(1000);
    }
    return NULL;
}

void* accel_thread(void* arg) {
    (void)arg;
    float prev_x = 0.0f, prev_y = 0.0f, prev_z = 0.0f;
    bool armed_x = true, armed_y = true, armed_z = true; // allow triggers
    uint64_t last_trigger_x = 0, last_trigger_y = 0, last_trigger_z = 0;

    {
        float ax=0, ay=0, az=0;
        if (Accelerometer_read(&ax, &ay, &az) == 0) {
            prev_x = ax; prev_y = ay; prev_z = az;  // initial read to establish baseline
        }
    }

    while (1) {
        float ax=0, ay=0, az=0;
        if (Accelerometer_read(&ax, &ay, &az) == 0) {
            Period_markEvent(PERIOD_EVENT_SAMPLE_ACCEL);
            // compute delta since last sample (simple high-pass / edge detector)
            float dx = ax - prev_x;
            float dy = ay - prev_y;
            float dz = az - prev_z;

            uint64_t t = now_ms();

            // X axis
            if (fabsf(dx) >= AXIS_THRESHOLD && armed_x && (t - last_trigger_x) > DEBOUNCE_MS) {
                // A hit on X
                play_axis_sound(AXIS_X);
                last_trigger_x = t;
                armed_x = false; // require release or debounce
            } else {
                // release if small movement detected
                if (fabsf(dx) <= AXIS_RELEASE && (t - last_trigger_x) > (DEBOUNCE_MS/2)) {
                    armed_x = true;
                }
            }

            // Y axis
            if (fabsf(dy) >= AXIS_THRESHOLD && armed_y && (t - last_trigger_y) > DEBOUNCE_MS) {
                play_axis_sound(AXIS_Y);
                last_trigger_y = t;
                armed_y = false;
            } else {
                if (fabsf(dy) <= AXIS_RELEASE && (t - last_trigger_y) > (DEBOUNCE_MS/2)) {
                    armed_y = true;
                }
            }

            // Z axis
            if (fabsf(dz) >= AXIS_THRESHOLD && armed_z && (t - last_trigger_z) > DEBOUNCE_MS) {
                play_axis_sound(AXIS_Z);
                last_trigger_z = t;
                armed_z = false;
            } else {
                if (fabsf(dz) <= AXIS_RELEASE && (t - last_trigger_z) > (DEBOUNCE_MS/2)) {
                    armed_z = true;
                }
            }

            // store for next sample
            prev_x = ax; prev_y = ay; prev_z = az;
        } else {
            fprintf(stderr, "Accelerometer read failed\n");
        }

        // 100 Hz polling
        usleep(POLL_MS * 1000);
    }
    return NULL;
}

void* udp_receive_thread(void* arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("udp recv socket"); return NULL; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("udp bind"); 
        close(sock); 
        return NULL;
    }

    printf("UDP receiver: listening on port %d\n", PORT);

    char buffer[256];
    struct sockaddr_in client;
        while (1) {
        socklen_t clientLen = sizeof(client);
        ssize_t len = recvfrom(sock, buffer, sizeof(buffer)-1, 0,
                               (struct sockaddr*)&client, &clientLen);
        if (len < 0) { 
            perror("udp recv error"); 
            continue; 
        }

        buffer[len] = '\0';
        printf("UDP received: '%s'\n", buffer);

        char reply[128];
        reply[0] = '\0';

        char* cmd = buffer;
        while (*cmd && isspace((unsigned char)*cmd)) cmd++;

        pthread_mutex_lock(&lock);

        if (strncmp(cmd, "status", 6) == 0) {
            snprintf(reply, sizeof(reply), "M%d %dbpm vol:%d", g_mode, g_bpm, g_volume);
        }
        else if (strncmp(cmd, "volume", 6) == 0) {
            int v;
            if (parse_int_safe(cmd + 6, &v)) { 
                if (v < 0) v = 0;
                if (v > 100) v = 100;
                g_volume = v; 
                AudioMixer_setVolume(g_volume);
            }
            snprintf(reply, sizeof(reply), "%d", g_volume);
        }
        else if (strncmp(cmd, "tempo", 5) == 0) {
            int t;
            if (parse_int_safe(cmd + 5, &t)) { 
                g_bpm = t; 
            }
            snprintf(reply, sizeof(reply), "%d", g_bpm);
        }
        else if (strncmp(cmd, "snare", 5) == 0) {
            AudioMixer_queueSound(&beatbox.data.rock.snare);
            snprintf(reply, sizeof(reply), "snare");
        }
        else if (strncmp(cmd, "kick", 4) == 0) {
            AudioMixer_queueSound(&beatbox.data.rock.kick);
            snprintf(reply, sizeof(reply), "kick");
        }
        else if (strncmp(cmd, "hihat", 5) == 0) {
            AudioMixer_queueSound(&beatbox.data.rock.hihat);
            snprintf(reply, sizeof(reply), "hihat");
        }
        else if (strncmp(cmd, "mode", 4) == 0) {
            int m;
            if (parse_int_safe(cmd + 4, &m)) {
                g_mode = m;
                beatbox.type = g_mode;
            }
            snprintf(reply, sizeof(reply), "%d", g_mode);
        } else if (strncmp(cmd, "play", 4) == 0) {
            if (cmd[4] == '0') {
                pthread_mutex_lock(&lock);
                BeatBox_playSample(&beatbox, 0); // kick
                pthread_mutex_unlock(&lock);
                snprintf(reply, sizeof(reply), "kick");
            } 
            else if (cmd[4] == '1') {
                pthread_mutex_lock(&lock);
                BeatBox_playSample(&beatbox, 2); // hi-hat
                pthread_mutex_unlock(&lock);
                snprintf(reply, sizeof(reply), "hihat");
            } 
            else if (cmd[4] == '2') {
                pthread_mutex_lock(&lock);
                BeatBox_playSample(&beatbox, 1); // snare
                pthread_mutex_unlock(&lock);
                snprintf(reply, sizeof(reply), "snare");
            } 
            else {
                snprintf(reply, sizeof(reply), "ERROR: unknown play command");
            }
        }
        else {
            snprintf(reply, sizeof(reply), "ERROR: unknown command");
        }

        pthread_mutex_unlock(&lock);

        // always send a reply
        sendto(sock, reply, strlen(reply), 0, (struct sockaddr*)&client, clientLen);
        printf("UDP sent reply: '%s'\n", reply);
    }
    close(sock);
    return NULL;
}

void* udp_thread(void* arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket failed");
        return NULL;
    }

    // Remote Node.js server
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("192.168.7.2");

    printf("UDP: sending to 127.0.0.1:%d\n", PORT);

    unsigned long lastPrint = time(NULL);

    while (1) {
        unsigned long now = time(NULL);

        if (now != lastPrint) {   // 1 Hz
            lastPrint = now;

            Period_statistics_t statsAudio, statsAccel;
            Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_AUDIO, &statsAudio);
            Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_ACCEL, &statsAccel);

            char buffer[256];
            snprintf(buffer, sizeof(buffer),
                     "Audio[min=%.2f max=%.2f avg=%.2f] "
                     "Accel[min=%.2f max=%.2f avg=%.2f]",
                     statsAudio.minPeriodInMs,
                     statsAudio.maxPeriodInMs,
                     statsAudio.avgPeriodInMs,
                     statsAccel.minPeriodInMs,
                     statsAccel.maxPeriodInMs,
                     statsAccel.avgPeriodInMs);

            ssize_t sent = sendto(
                sock,
                buffer,
                strlen(buffer),
                0,
                (struct sockaddr*)&serverAddr,
                sizeof(serverAddr)
            );

            if (sent < 0) {
                perror("UDP send error");
            } else {
                printf("UDP sent: %s\n", buffer);
            }
        }

        usleep(1000);
    }

    close(sock);
    return NULL;
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

    // Initialize hardware and period timer
    Period_init();
    AudioMixer_init();
    BeatBox_init(&beatbox);
    encoder_init();
    accelerometer_init();

    pthread_t drum, enc, js, ac, udp, udp_rec;
    pthread_create(&ac, NULL, accel_thread, NULL);
    pthread_create(&enc, NULL, encoder_thread, NULL);
    pthread_create(&js, NULL, joystick_thread, NULL);
    pthread_create(&udp_rec, NULL, udp_receive_thread, NULL);
    pthread_create(&udp, NULL, udp_thread, NULL);
    pthread_create(&drum, NULL, drum_thread, NULL);

    pthread_join(udp, NULL);
    pthread_join(udp_rec, NULL);
    pthread_join(drum, NULL);
    pthread_join(enc, NULL);
    pthread_join(js, NULL);
    pthread_join(ac, NULL);

    BeatBox_cleanup(&beatbox);
    AudioMixer_cleanup();
    clean_encoder();

    return 0;
}
