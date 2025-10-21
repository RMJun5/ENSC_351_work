#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h> 
#include <linux/spi/spidev.h>
#include "reaction_timer.h"
int main(void){
    srand(getTimeInMs()&1000);
    bool want_up = rand() % 2; // randomly choose up or down 
    const char* dev = "/dev/spidev0.0";
    uint8_t mode = 0;
    uint8_t bits = 8;
    uint32_t speed_hz = 250000;
    
   /* open joystick (channels 0 = X, 1 = Y) */
    int fd = open(dev,O_RDWR);
    calibrate_joy(fd);
     if (fd < 0) { perror("open"); return 1; }
     if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) { perror("mode"); return 1; }
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) { perror("bpw"); return 1; }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) == -1) { perror("speed"); return 1;}

    int ch0 = joy_read_ch(fd, 0, speed_hz); 
    int ch1 = joy_read_ch(fd, 1, speed_hz);

    joystick_direction_t dir = joy_get_direction(ch0,ch1);

    //Say hello 
    printf("Hello embedded world, from Richard!");
    
    int best_ms = -1;
    
    
    while (1) {
         // get ready message
      printf("Now.... Get ready! \n");
      // flash LEDs 4 times to signal start of test
    // Flash LEDs 4 times to signal start
        flash_led(g_set_act_on, 4, nanotoms(250));  // 250 ms total duration
        flash_led(r_set_act_on, 4, nanotoms(250));
        // Step 2: if user is pressing up or down tell them to let go (just once), then wait until neutral 
       
        if (dir != DIR_NONE) {
        printf("Please let go of joystick\n");
            /* Wait until neither axis is beyond threshold */
            while (dir!= DIR_NONE) {
                int x = joy_read_ch(fd, JOY_DEFAULT_AXIS_X, speed_hz);
                int y = joy_read_ch(fd, JOY_DEFAULT_AXIS_Y, speed_hz);
                if (x < 0 || y < 0) {
                    fprintf(stderr, "joystick read error\n");
                    close(fd);
                    return 1;
                }
                if (dir== DIR_NONE) {
                    break; /* neutral */
                }
            }
        }
         /* Step 3: Wait random 0.5..3.0s */
        long long delay_ms = (rand() % (3000 - 500 + 1)) + 500; /* inclusive 500..3000 */
        sleep_ms(delay_ms);
        // Step 4: After delay, check if joystick is still pressed (too soon)
        int post_x = joy_read_ch(fd, JOY_DEFAULT_AXIS_X,speed_hz);
        int post_y = joy_read_ch(fd, JOY_DEFAULT_AXIS_Y, speed_hz);
        if (post_x < 0 || post_y < 0) {
            fprintf(stderr, "Error reading joystick post-delay\n");
            close(fd);
            return 1;
        }
        if (dir !=DIR_NONE) {
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
        long long t_start = getTimeInMs();
        
    while (getTimeInMs()-t_start<5000) {
        if (dir!=DIR_NONE){
            long long t_now = getTimeInMs(); 
            long long elapsed_ms = runtime(t_now,t_start);
            // Determine correctness
            bool correct = false;

        switch (dir) {
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
                printf("THANKS FOR PLAYING");
                cleanup();
                return 1;
            }
    
    // Update best time if correct
    if (correct) {
        if (best_ms < 0 || elapsed_ms < best_ms) {
            best_ms = elapsed_ms;
            printf("New best time: %dms\n", best_ms);
        }
    
        printf("This attempt: %lldms. Best: %dms\n", elapsed_ms, best_ms);
        flash_led(g_set_act_on, 5, 1000);  // flash green
        } else if (dir != DIR_NONE) {
        // Flash red for wrong input
        printf("Correct direction was %s.\n", want_up ? "UP" : "DOWN");
        flash_led(r_set_act_on, 5, 1000);  // flash red
        }
    
     }else{
            printf("Took too long (>5s), Exiting");
            cleanup();
            return 1;
        }
    }
    return 0;
      
}


