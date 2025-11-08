#define _GNU_SOURCE
#include "hal/encoder.h"

unsigned int LINE_OFFSET = 7;      // The offset of the GPIO line you want to control (e.g., GPIO16)
int num;

struct gpiod_chip *chip;
struct gpiod_line_settings *settings;
struct gpiod_line_config *config;
struct gpiod_request_config *req_cfg;
struct gpiod_line_request *request;

const char *CHIPNAME = "/dev/gpiochip2"; // Typically the name of your GPIO chip


/**
 * @brief Reads the encoder
 * 
 */
void encoder_init() {

    // 1. open the gpiochip0
    chip = gpiod_chip_open(CHIPNAME);
    if (chip == NULL){
        perror("Cannot open the chip\n");
        gpiod_chip_close(chip);
        return;
    }
    printf("%s", "Chip opened\n");


   // 2. Create line settings (INPUT for encoder signals)
    settings = gpiod_line_settings_new();
    if (settings == NULL){
        perror("Cannot get line settings");
        gpiod_chip_close(chip);
        return;
    }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);  // <-- Here!

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
    // int value = gpiod_line_request_get_value(request, LINE_OFFSET);
    // if (value < 0) {
    //     perror("gpiod_line_request_get_value");
    // } else {
    //     printf("Encoder line %u value: %d\n", LINE_OFFSET, value);
    // }

    printf("%s", "Got the chips info");

    // 6. Close the chip
    gpiod_chip_close(chip);

}

int read_encoder() {
    if (!request) return -1;
    int value = gpiod_line_request_get_value(request, LINE_OFFSET);
    if (value < 0)
        perror("Failed to read encoder line");
    return value;
}