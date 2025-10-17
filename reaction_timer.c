#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <led.h>
#include <joystick.h>
#include <reaction_timer.h>
#define JOY_CALIBRATION_ENABLED

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
bool want_up = (rand() % 2) == 0; // randomly choose up or down 
const char* dev = "/dev/spidev0.0";
uint8_t mode = 0; // SPI mode 0
uint8_t bits = 8;
uint32_t speed = 250000;
    // open joystick device
    int fd = joy_open(dev, speed, mode, bits);
    if(fd<0) {
        perror("joy_open");
        return 1;
    }
    //Say hello 
    printf("Hello embedded world, from Richard!");
    
    /* open joystick (channels 0 = X, 1 = Y) */
    joystick_t *joy = joy_open(JOY_DEFAULT_DEVICE, JOY_DEFAULT_SPEED_HZ,
                               JOY_DEFAULT_AXIS_X, JOY_DEFAULT_AXIS_Y);
    if (!joy) {
        perror("joy_open");
        return 1;
    }
    const int THRESHOLD = 600;       /* raw units above center to be considered pressed — tune this */
    int best_ms = -1;

    while (1) {
         // get ready message
      printf("Now.... Get ready! \n");
      // flash LEDs 4 times to signal start of test
      for (int i = 0; i < 4; ++i) {
            // Green LED on for 250 m/s then off
            if (g_set_act_on(1) != 0) { perror("g_set_act_on"); }
            sleep_ms(250);
            g_set_act_on(0);
            // Red LED on for 250 m/s then off
            if (r_set_act_on(1) != 0) { perror("r_set_act_on"); }
            sleep_ms(250);
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
        if (abs(base_x - (1 << (JOY_ADC_BITS-1))) > THRESHOLD ||
            abs(base_y - (1 << (JOY_ADC_BITS-1))) > THRESHOLD) {
            printf("Please let go of joystick\n");
            /* Wait until neither axis is beyond threshold */
            while (1) {
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
        int pressed_ch = -1;
        int elapsed_ms = 0;

        while (1) {
            int x = joy_read_raw(joy, JOY_DEFAULT_AXIS_X);
            int y = joy_read_raw(joy, JOY_DEFAULT_AXIS_Y);
            if (x < 0 || y < 0) {
                fprintf(stderr, "Joystick read error during timing\n");
                joy_close(joy);
                return 1;
            }

            /* check press — whichever axis exceeds threshold */
            if (abs(x - (1 << (JOY_ADC_BITS-1))) > THRESHOLD) pressed_ch = JOY_DEFAULT_AXIS_X;
            else if (abs(y - (1 << (JOY_ADC_BITS-1))) > THRESHOLD) pressed_ch = JOY_DEFAULT_AXIS_Y;

            clock_gettime(CLOCK_MONOTONIC, &t_now);
            elapsed_ms = (int)((t_now.tv_sec - t_start.tv_sec) * 1000 +
                               (t_now.tv_nsec - t_start.tv_nsec) / 1000000);

            if (pressed_ch != -1) break;
            if (elapsed_ms > 5000) {
                printf("Too long! Exiting!\n");
                g_set_act_on(0);
                r_set_act_on(0);
                joy_close(joy);
                return 1;
            }
            sleep_ms(10);
        }

        // Turn off the LED that was lit
        g_set_act_on(0);
        r_set_act_on(0);
         /* Determine correctness */
        bool pressed_up = (pressed_ch == JOY_DEFAULT_AXIS_X);
        bool pressed_down = (pressed_ch == JOY_DEFAULT_AXIS_Y);

        if (pressed_up && want_up) {
            printf("Correct! You moved UP.\n");
            /* best time handling */
            if (best_ms < 0 || elapsed_ms < best_ms) {
                best_ms = elapsed_ms;
                printf("New best time: %d ms\n", best_ms);
            }
            printf("This attempt: %d ms. Best attempt: %d ms\n", elapsed_ms, best_ms);
            /* flash green 5 times in 1000ms total */
            flash_led(g_set_act_on, 5, 1000);
            g_set_act_on(0);
            r_set_act_on(0);
        } else if (pressed_down && !want_up) {
            printf("Correct! You moved DOWN.\n");
            if (best_ms < 0 || elapsed_ms < best_ms) {
                best_ms = elapsed_ms;
                printf("New best time: %d ms\n", best_ms);
            }
            printf("This attempt: %d ms. Best: %d ms\n", elapsed_ms, best_ms);
            flash_led(g_set_act_on, 5, 1000); /* flash green on success */
            g_set_act_on(0);
            r_set_act_on(0);
        } else if (pressed_up || pressed_down) {
            /* pressed opposite direction (incorrect) */
            printf("Incorrect direction! You pressed %s.\n", pressed_up ? "UP" : "DOWN");
            printf("correct direction was %s.\n", want_up ? "UP" : "DOWN");
            flash_led(r_set_act_on, 5, 1000); /* flash red 5 times in 1s */
            g_set_act_on(0);
            r_set_act_on(0);
        } else {
            /* Shouldn't happen: other axis (left/right) or unknown */
            printf("User chose to quit(right/left input). Bye!\n");
            joy_close(joy);
            return 1;
        }

        /* small gap before next round */
        sleep_ms(500);
    /* unreachable, but clean up */
    joy_close(joy);
    return 0;
}


