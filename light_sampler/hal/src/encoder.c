#define _GNU_SOURCE
#include "hal/encoder.h"
#include <errno.h>
#include <string.h>
// GPIO 27 and 22
unsigned int LINE_OFFSET[] = {33, 41};      // The offset of the GPIO line you want to control (e.g., GPIO16)
int num;

struct gpiod_chip *chip;
struct gpiod_line_settings *settings;
struct gpiod_line_config *config;
struct gpiod_request_config *req_cfg;
struct gpiod_line_request *request;

const char *CHIPNAME = "/dev/gpiochip1"; // The GPIO chip for pins GPIO 22 and 27


/**
 * @brief Initialize the encoder
 * 
 */
void encoder_init() {
    chip = NULL;
    settings = NULL;
    config = NULL;
    req_cfg = NULL;
    request = NULL;

    // 1. open the gpiochip0
    chip = gpiod_chip_open(CHIPNAME);
    if (chip == NULL){
        printf("%s", "Cannot open the chip\n");
        goto error;
    }
    printf("%s", "Chip opened\n");


   // 2. Create line settings (INPUT for encoder signals)
    settings = gpiod_line_settings_new();
    if (!settings){
        printf("%s", "Cannot get line settings");
        goto error;
    }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    //gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH); // Edge detection
    
    // 3. Create line config and request the line
    config = gpiod_line_config_new();
    if (!config){
        printf("%s", "Cannot get line config");
        goto error;
    }
    gpiod_line_config_add_line_settings(config, LINE_OFFSET, 2, settings);


    // 4. Create request config
    req_cfg = gpiod_request_config_new();
    if (!req_cfg){
        printf("%s", "Cannot get request config");
        goto error;
    }
    gpiod_request_config_set_consumer(req_cfg, "encoder"); // myapp


    // 5. Request the lines
    request = gpiod_chip_request_lines(chip, req_cfg, config);
    if (!request){
        printf("%s", "Cannot get request");
        goto error;
    }

    printf("Encoder initialized on GPIOs %u and %u\n", LINE_OFFSET[0], LINE_OFFSET[1]);
    return;

    error:
        clean_encoder(); // cleans only what’s been allocated so far
        return;
}

int read_encoder() {
    if (!request) return -1;

    int values[2];
    int ret = gpiod_line_request_get_values(request, values);
    if (ret < 0) {
        printf("Failed to read encoder lines: %s\n", strerror(errno));
        return -1;
    }
    //printf("A=%d, B=%d\n", values[0], values[1]);

    // Pack A/B into a single number (bitwise)
    return (values[0] << 1) | values[1];

    // instead, return the values

}



void clean_encoder() {

    printf("clean_encoder() called — closing GPIO chip now\n");

    if (request != NULL) {
        gpiod_line_request_release(request);
        request = NULL;
    }
    if (req_cfg != NULL) {
        gpiod_request_config_free(req_cfg);
        req_cfg = NULL;
    }
    if (config != NULL) {
        gpiod_line_config_free(config);
        config = NULL;
    }
    if (settings != NULL) {
        gpiod_line_settings_free(settings);
        settings = NULL;
    }
    if (chip != NULL) {
        gpiod_chip_close(chip);
        chip = NULL;
    }
    return;
}