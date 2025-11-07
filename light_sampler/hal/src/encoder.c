#define _GNU_SOURCE
#include "hal/encoder.h"

unsigned int line_offset = 16;      // The offset of the GPIO line you want to control (e.g., GPIO16)
struct gpiod_chip *chip;
struct gpiod_line_settings *settings;
struct gpiod_line_config *config;
struct gpiod_request_config *req_cfg;
int num;
struct gpiod_chip_info *info;
struct gpiod_line_request *request;
const char *CHIPNAME = "/dev/gpiochip0"; // Typically the name of your GPIO chip


/**
 * @brief Reads the encoder
 * 
 */
void read_encoder() {
 
    // open the gpiochip0
    chip = gpiod_chip_open(CHIPNAME);
    if (chip == NULL){
        perror("Cannot open the chip");
        gpiod_chip_close(chip);
        return;
    }
    printf("%s", "Chip opened");


    settings = gpiod_line_settings_new();
    if (settings == NULL){
        perror("Cannot get line settings");
        gpiod_chip_close(chip);
        return;
    }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);


    config = gpiod_line_config_new();
    if (config == NULL){
        perror("Cannot get line config");
        gpiod_chip_close(chip);
        return;
    }

    int offset = line_offset;
    if (offset < 0){
        perror("Cannot get offset");
        gpiod_chip_close(chip);
        return;
    }

    gpiod_line_config_add_line_settings(config, &offset, 1, settings);


    req_cfg = gpiod_request_config_new();
    if (req_cfg == NULL){
        perror("Cannot get request config");
        gpiod_chip_close(chip);
        return;
    }
    request = gpiod_chip_request_lines(chip, req_cfg, config);
    if (request == NULL){
        perror("Cannot get request");
        gpiod_chip_close(chip);
        return;
    }
    gpiod_request_config_set_consumer(req_cfg, "myapp");

    printf("%s", "Got the chips info");

    // Direction direction;
    // if (0) {}

    // // Free the memory
    gpiod_line_request_release(request);
    gpiod_chip_close(chip);

    // return NONE;
}

