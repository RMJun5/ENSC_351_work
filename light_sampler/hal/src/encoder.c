
#include "hal/encoder.h"
#include <gpiod.h>

unsigned int line_offset = 16;      // The offset of the GPIO line you want to control (e.g., GPIO16)
struct gpiod_chip *chip;
struct gpiod_line *line;
int num;
struct gpiod_chip_info *info;
struct gpiod_line_request *request;
const char *CHIPNAME = "gpiochip0"; // Typically the name of your GPIO chip

void read_encoder() {
 
    // // open the gpiochip0
    chip = gpiod_chip_open(CHIPNAME);
    if (chip == NULL){
        perror("Cannot open the chip");
        gpiod_chip_close(chip);
        return;
    }
    
    // // Get the specific GPIO line
    info = gpiod_chip_get_info(chip);
    if (line == NULL){
        perror("Cannot fetch line");
        gpiod_chip_close(chip);
        return;
    }


    // Direction direction;
    // if (0) {}

    // // Free the memory
    gpiod_line_request_release(request);
    gpiod_chip_close(chip);

    // return NONE;
}