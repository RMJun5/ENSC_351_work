#define _POSIX_C_SOURCE 199309L  // MUST come before any #include
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <led.h>
#include <joystick.h>
#include <reaction_timer.h>
#define JOY_CALIBRATION_ENABLED
static long long getTimeInMs(int ns)
{
struct timespec spec;
clock_gettime(CLOCK_REALTIME, &spec);
long long seconds = spec.tv_sec;
long long nanoSeconds = spec.tv_nsec;
long long milliSeconds = seconds * 1000
+ nanoSeconds / 1000000;
return milliSeconds;
}
static void sleep_ms(long long delayInMs)
{
const long long NS_PER_MS = 1000 * 1000;
const long long NS_PER_SECOND = 1000000000;
long long delayNs = delayInMs * NS_PER_MS;
int seconds = delayNs / NS_PER_SECOND;
int nanoseconds = delayNs % NS_PER_SECOND;
struct timespec reqDelay = {seconds, nanoseconds};
nanosleep(&reqDelay, (struct timespec *) NULL);
}

#ifdef JOY_CALIBRATION_ENABLED
static void calibrate_joystick(joystick_t *joy) {
/* quick interactive calibration: ask user to wiggle the stick for 3 seconds */
printf("Calibration: please move the joystick through its full range for 3 seconds...\n");
joy_calib_start(joy);
const int CAL_MS = 3000;
struct timespec t0, tnow;
clock_gettime(CLOCK_MONOTONIC, &t0);
while (1) {
    int rawx = joy_read_raw(joy, JOY_DEFAULT_AXIS_X);
    int rawy = joy_read_raw(joy, JOY_DEFAULT_AXIS_Y);
    if (rawx >= 0 && rawy >= 0) {
        joy_calib_update(joy, rawx, rawy);
    }
    nanosleep(&(struct timespec){0, 20000000L}, NULL); /* 20 ms */
    clock_gettime(CLOCK_MONOTONIC, &tnow);
    long elapsed_ms = (tnow.tv_sec - t0.tv_sec) * 1000 + (tnow.tv_nsec - t0.tv_nsec) / 1000000;
    if (elapsed_ms >= CAL_MS) break;
}
joy_calib_finalize(joy);
joy_calib_save(joy, "/tmp/joycalib.txt");   
printf("Calibration done. minX=%d maxX=%d centerX=%d\n", joy->calib.min_x, joy->calib.max_x, joy->calib.center_x);
}
#endif /* JOY_CALIBRATION_ENABLED */


int main(void){
srand(time(NULL)); // seed random numbers
bool want_up = (rand() % 2) == 0; // randomly choose up or down 
const char* dev = "/dev/spidev0.0";
uint8_t ch0 = 0;
uint8_t ch1 = 1;
uint8_t mode = 0;
uint8_t bits = 8;
uint32_t speed_hz = 250000;
   /* open joystick (channels 0 = X, 1 = Y) */
    joystick_t *joy = joy_open(dev, speed_hz, mode, bits, ch0, ch1);
    if (!joy) {
        perror("joy_open");
        return 1;
    }
     // Initialize joystick module
    if (!joystick_init("/dev/spidev0.0", 250000, 0, 8, 0, 1)) {
        perror("joystick_init failed");
        return 1;
    }

    //Say hello 
    printf("Hello embedded world, from Richard!");
    
 
    const int THRESHOLD = 600;       /* raw units above center to be considered pressed — tune this */
    int best_ms = -1;

    while (1) {
         // get ready message
      printf("Now.... Get ready! \n");
      // flash LEDs 4 times to signal start of test
      for (int i = 0; i < 4; ++i) {
            // Green LED on for 250 m/s then off
            if (g_set_act_on(1) != 0) { perror("g_set_act_on"); }
            sleep_ms(getTimeInMs(250000000));
            g_set_act_on(0);
            // Red LED on for 250 m/s then off
            if (r_set_act_on(1) != 0) { perror("r_set_act_on"); }
            sleep_ms(getTimeInMs(250000000));
            r_set_act_on(0);
        }
         /* read neutral baseline once */
        int base_x = joy_read_raw(joy, JOY_DEFAULT_AXIS_X);
        int base_y = joy_read_raw(joy, JOY_DEFAULT_AXIS_Y);
        if (base_x < 0 || base_y < 0) {
            fprintf(stderr, "Error reading joystick baseline\n");
            joy_close(joy);
            return 1;
        }
        /* Step 2: if user is pressing up or down tell them to let go (just once), then wait until neutral */
        joystick_direction_t dir = joystick_get_direction();
    if (dir != DIR_NONE) {
    printf("Please let go of joystick\n");
     /* Wait until neither axis is beyond threshold */
            while (joystick_get_direction()!= DIR_NONE) {
                sleep_ms(100);
                int x = joy_read_raw(joy, JOY_DEFAULT_AXIS_X);
                int y = joy_read_raw(joy, JOY_DEFAULT_AXIS_Y);
                if (x < 0 || y < 0) {
                    fprintf(stderr, "Joystick read error\n");
                    joy_close(joy);
                    return 1;
                }
                if (abs(x - (1 << (JOY_ADC_BITS-1))) <= THRESHOLD &&
                    abs(y - (1 << (JOY_ADC_BITS-1))) <= THRESHOLD) {
                    break; /* neutral */
                }
            }
        }
         /* Step 3: Wait random 0.5..3.0s */
        int delay_ms = (rand() % (3000 - 500 + 1)) + 500; /* inclusive 500..3000 */
        sleep_ms(delay_ms);
        // Step 4: After delay, check if joystick is still pressed (too soon)
        int post_x = joy_read_raw(joy, JOY_DEFAULT_AXIS_X);
        int post_y = joy_read_raw(joy, JOY_DEFAULT_AXIS_Y);
        if (post_x < 0 || post_y < 0) {
            fprintf(stderr, "Error reading joystick post-delay\n");
            joy_close(joy);
            return 1;
        }
        if (abs(post_x - (1 << (JOY_ADC_BITS-1))) > THRESHOLD ||
            abs(post_y - (1 << (JOY_ADC_BITS-1))) > THRESHOLD) {
            printf("Too soon! Back to start\n");
            continue; /* back to start of while(1) */
        }
        //Step 5: pick a random direction (up or down) and prompt user to move joystick
        printf("Move joystick %s now!\n", want_up ? "UP" : "DOWN");

        // Turn on appropriate LED
        if (want_up) g_set_act_on(1);
        else r_set_act_on(1);
        }
        // measure time until user presses (any direction) or timeout (5s)
        struct timespec t_start, t_now;
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        joystick_direction_t pressed_dir = DIR_NONE;
struct timespec t_start;
clock_gettime(CLOCK_MONOTONIC, &t_start);
int elapsed_ms = 0;

    while (pressed_dir == DIR_NONE && elapsed_ms < 5000) {
        pressed_dir = joystick_get_direction();
        struct timespec t_now;
        clock_gettime(CLOCK_MONOTONIC, &t_now);
        elapsed_ms = (int)((t_now.tv_sec - t_start.tv_sec) * 1000 +
                       (t_now.tv_nsec - t_start.tv_nsec) / 1000000);
        sleepForMs(10);
    }

    if (pressed_dir == DIR_NONE) {
        printf("Too long! Exiting!\n");
        joystick_cleanup();
        return 1;
    }

       // Turn off any lit LEDs before checking correctness
g_set_act_on(0);
r_set_act_on(0);

// Determine which direction the user pressed
joystick_direction_t pressed_dir = joystick_get_direction();

// Determine correctness
bool correct = false;

switch (pressed_dir) {
    case DIR_UP:
        if (want_up) {
            correct = true;
            printf("Correct! You moved UP.\n");
        } else {
            printf("Incorrect! You pressed UP.\n");
        }
        break;
    case DIR_DOWN:
        if (!want_up) {
            correct = true;
            printf("Correct! You moved DOWN.\n");
        } else {
            printf("Incorrect! You pressed DOWN.\n");
        }
        break;
    case DIR_NONE:
        printf("No direction pressed. Unexpected!\n");
        break;
    default:
        printf("Other direction pressed (left/right). Exiting!\n");
        joystick_cleanup();
        return 1;
}

    // Update best time if correct
    if (correct) {
        if (best_ms < 0 || elapsed_ms < best_ms) {
            best_ms = elapsed_ms;
            printf("New best time: %d ms\n", best_ms);
        }
        printf("This attempt: %d ms. Best: %d ms\n", elapsed_ms, best_ms);
        flash_led(g_set_act_on, 5, 1000);  // flash green
    } else if (pressed_dir != DIR_NONE) {
        // Flash red for wrong input
        printf("Correct direction was %s.\n", want_up ? "UP" : "DOWN");
        flash_led(r_set_act_on, 5, 1000);  // flash red
    }

        /* small gap before next round */
        sleep_ms(500);
    /* unreachable, but clean up */
    joy_close(joy);
    joystick_cleanup();
    return 0;
}


