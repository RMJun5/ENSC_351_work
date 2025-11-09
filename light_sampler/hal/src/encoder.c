#define _GNU_SOURCE
#include "hal/encoder.h"

unsigned int LINE_OFFSET[] = {7, 8};      // The offset of the GPIO line you want to control (e.g., GPIO16)
int num;

struct gpiod_chip *chip;
struct gpiod_line_settings *settings;
struct gpiod_line_config *config;
struct gpiod_request_config *req_cfg;
struct gpiod_line_request *request;

const char *CHIPNAME = "/dev/gpiochip2"; // The GPIO chip for pins GPIO 16 and 17


/**
 * @brief Initialize the encoder
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
    if (!settings){
        perror("Cannot get line settings");
        gpiod_chip_close(chip);
        return;
    }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
    //gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH); // Edge detection
    
    // 3. Create line config and request the line
    config = gpiod_line_config_new();
    if (!config){
        perror("Cannot get line config");
        gpiod_chip_close(chip);
        return;
    }
    gpiod_line_config_add_line_settings(config, LINE_OFFSET, 2, settings);


    // 4. Create request config
    req_cfg = gpiod_request_config_new();
    if (!req_cfg){
        perror("Cannot get request config");
        gpiod_chip_close(chip);
        return;
    }
    gpiod_request_config_set_consumer(req_cfg, "encoder"); // myapp


    // 5. Request the lines
    request = gpiod_chip_request_lines(chip, req_cfg, config);
    if (!request){
        perror("Cannot get request");
        gpiod_chip_close(chip);
        return;
    }

    printf("Encoder initialized on GPIOs %u and %u\n", LINE_OFFSET[0], LINE_OFFSET[1]);

}

int read_encoder() {
    if (!request) return -1;

    int values[2];
    int ret = gpiod_line_request_get_values(request, values);

    if (ret < 0) {
        perror("Failed to read encoder lines");
        return -1;
    }
    printf("A=%d, B=%d\n", values[0], values[1]);

    // Pack A/B into a single number (bitwise)
    return (values[0] << 1) | values[1];
}

void clean_encoder() {
    if (request) {
        gpiod_line_request_release(request);
        request = NULL;
    }
    if (req_cfg) {
        gpiod_request_config_free(req_cfg);
        req_cfg = NULL;
    }
    if (config) {
        gpiod_line_config_free(config);
        config = NULL;
    }
    if (settings) {
        gpiod_line_settings_free(settings);
        settings = NULL;
    }
    if (chip) {
        gpiod_chip_close(chip);
        chip = NULL;
    }
    return;
}