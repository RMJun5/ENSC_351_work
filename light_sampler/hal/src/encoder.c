
#include "hal/encoder.h"

unsigned int line_offset = 16;      // The offset of the GPIO line you want to control (e.g., GPIO16)
struct gpiod_chip *chip;
struct gpiod_line *line;

Direction read_encoder() {

    // open the gpiochip0
    chip = gpiod_chip_open_by_name(CHIPNAME);
    if (chip == NULL){
        perror("Cannot open the chip");
        gpiod_chip_close(chip);
        return -1;
    }
    
    // Get the specific GPIO line
    line = gpiod_chip_get_line(chip, line_offset);
    if (line == NULL){
        perror("Cannot fetch line");
        gpiod_chip_close(chip);
        return -1;
    }


    Direction direction;
    if (0) {}

    // Free the memory
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return NONE;
}

