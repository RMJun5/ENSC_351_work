#define _POSIX_C_SOURCE 199309L  // MUST come before any #include
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <reaction_timer.h>

int main(void){


    srand(time(NULL)); // seed random numbers
    bool want_up = (rand() % 2) == 0; // randomly choose up or down 
    const char* dev = "~/dev/spidev0.0";
    uint8_t ch0 = 0;
    uint8_t ch1 = 1;
    uint8_t mode = 0;
    uint8_t bits = 8;
    uint32_t speed_hz = 250000;
    
   /* open joystick (channels 0 = X, 1 = Y) */
    joystick_t* fd = joy_open(dev, speed_hz, mode, bits, ch0, ch1);
    if (!fd) {
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
    
 
    const int THRESHOLD = 600;       /* raw units above center to be considered pressed — tune if needed */
    int best_ms = -1;

    while (1) {
         // get ready message
      printf("Now.... Get ready! \n");
      // flash LEDs 4 times to signal start of test
    // Flash LEDs 4 times to signal start
        flash_led(g_set_act_on, 4, 250);  // 250 ms total duration
        flash_led(r_set_act_on, 4, 250);

         /* read neutral baseline once */
        int base_x = joy_read_raw(fd, JOY_DEFAULT_AXIS_X);
        int base_y = joy_read_raw(fd, JOY_DEFAULT_AXIS_Y);
        if (base_x < 0 || base_y < 0) {
            fprintf(stderr, "Error reading joystick baseline\n");
            joy_close(fd);
            return 1;
        }
        // Step 2: if user is pressing up or down tell them to let go (just once), then wait until neutral 
        joystick_direction_t dir = joystick_get_direction();
    if (dir != DIR_NONE) {
    printf("Please let go of joystick\n");
     /* Wait until neither axis is beyond threshold */
            while (joystick_get_direction()!= DIR_NONE) {
                sleep_ms(100);
                int x = joy_read_raw(fd, JOY_DEFAULT_AXIS_X);
                int y = joy_read_raw(fd, JOY_DEFAULT_AXIS_Y);
                if (x < 0 || y < 0) {
                    fprintf(stderr, "joystick read error\n");
                    joy_close(fd);
                    return 1;
                }
                if (joystick_get_direction()== DIR_NONE) {
                    break; /* neutral */
                }
            }
        }
         /* Step 3: Wait random 0.5..3.0s */
        int delay_ms = (rand() % (3000 - 500 + 1)) + 500; /* inclusive 500..3000 */
        sleep_ms(delay_ms);
        // Step 4: After delay, check if joystick is still pressed (too soon)
        int post_x = joy_read_raw(fd, JOY_DEFAULT_AXIS_X);
        int post_y = joy_read_raw(fd, JOY_DEFAULT_AXIS_Y);
        if (post_x < 0 || post_y < 0) {
            fprintf(stderr, "Error reading joystick post-delay\n");
            joy_close(fd);
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
        joystick_direction_t pressed_dir = joystick_get_direction();
struct timespec t_start;
clock_gettime(CLOCK_MONOTONIC, &t_start);
int elapsed_ms = 0;

    while (pressed_dir == DIR_NONE && elapsed_ms < 5000) {
        pressed_dir = joystick_get_direction();
        struct timespec t_now;
        clock_gettime(CLOCK_MONOTONIC, &t_now);
        elapsed_ms = (int)((t_now.tv_sec - t_start.tv_sec) * 1000 +
                       (t_now.tv_nsec - t_start.tv_nsec) / 1000000);
        sleep_ms(10);
    }

    if (pressed_dir == DIR_NONE) {
        printf("Too long! Exiting!\n");
        joystick_cleanup();
        return 1;
    }

       // Turn off any lit LEDs before checking correctness
g_set_act_on(0);
r_set_act_on(0);


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
    joy_close(fd);
    joystick_cleanup();
    return 0;
}


