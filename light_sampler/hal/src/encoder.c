#define _GNU_SOURCE
#include "hal/encoder.h"

unsigned int LINE_OFFSET = 23;      // The offset of the GPIO line you want to control (e.g., GPIO16)
int num;

const char *CHIPNAME = "/dev/gpiochip0"; // Typically the name of your GPIO chip


/**
 * @brief Reads the encoder
 * 
 */
void read_encoder() {

    struct gpiod_chip *chip;
    struct gpiod_line_settings *settings;
    struct gpiod_line_config *config;
    struct gpiod_request_config *req_cfg;
    struct gpiod_line_request *request;

    // 1. open the gpiochip0
    chip = gpiod_chip_open(CHIPNAME);
    if (chip == NULL){
        perror("Cannot open the chip");
        gpiod_chip_close(chip);
        return;
    }
    printf("%s", "Chip opened");


   // 2. Create line settings (INPUT for encoder signals)
    settings = gpiod_line_settings_new();
    if (settings == NULL){
        perror("Cannot get line settings");
        gpiod_chip_close(chip);
        return;
    }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);


    // 3. Create line config and request the line
    config = gpiod_line_config_new();
    if (config == NULL){
        perror("Cannot get line config");
        gpiod_chip_close(chip);
        return;
    }
    gpiod_line_config_add_line_settings(config, &LINE_OFFSET, 1, settings);


    // 4. Create request config and request the line
    req_cfg = gpiod_request_config_new();
    if (req_cfg == NULL){
        perror("Cannot get request config");
        gpiod_chip_close(chip);
        return;
    }
    gpiod_request_config_set_consumer(req_cfg, "myapp");
    
    request = gpiod_chip_request_lines(chip, req_cfg, config);
    if (request == NULL){
        perror("Cannot get request");
        gpiod_chip_close(chip);
        return;
    }

    // 5. Read the current logical value of the line
    int value = gpiod_line_request_get_value(request, LINE_OFFSET);
    if (value < 0) {
        perror("gpiod_line_request_get_value");
    } else {
        printf("Encoder line %u value: %d\n", LINE_OFFSET, value);
    }

    printf("%s", "Got the chips info");

    // 6. Close the chip
    gpiod_chip_close(chip);

}

