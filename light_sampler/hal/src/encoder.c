#define _GNU_SOURCE
#include "hal/encoder.h"

unsigned int line_offset = 16;      // The offset of the GPIO line you want to control (e.g., GPIO16)
struct gpiod_chip *chip;
struct gpiod_line_info *line;
int num;
struct gpiod_chip_info *info;
struct gpiod_line_request *request;
const char *CHIPNAME = "/dev/gpiochip0"; // Typically the name of your GPIO chip


/**
 * @brief Reads the encoder
 * 
 */
void read_encoder() {
 
    // // open the gpiochip0
    chip = gpiod_chip_open(CHIPNAME);
    if (chip == NULL){
        perror("Cannot open the chip");
        gpiod_chip_close(chip);
        return;
    }
    printf("%s", "Chip opened");
    
    // // Get the specific GPIO line
    line = gpiod_chip_get_line_info(chip, 23);
    if (line == NULL){
        perror("Cannot fetch line");
        gpiod_chip_close(chip);
        return;
    }
    printf("%s", "Got the chips info");

    // Direction direction;
    // if (0) {}

    // // Free the memory
    gpiod_line_request_release(request);
    gpiod_chip_close(chip);

    // return NONE;
}

