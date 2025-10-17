#pragma once

#include <stdint.h>
#include <stdbool.h>


/* Default SPI device and default ADC channels for X/Y */
#define JOY_DEFAULT_DEVICE    "/dev/spidev0.0"
#define JOY_DEFAULT_SPEED_HZ  250000U
#define JOY_DEFAULT_AXIS_X    0   /* ADC channel for X axis (0..7) */
#define JOY_DEFAULT_AXIS_Y    1   /* ADC channel for Y axis (0..7) */

/* Expected ADC resolution used by normalization (12 => 0..4095).
   Set to 10 if using a 10-bit ADC (0..1023). */
#define JOY_ADC_BITS          12

/* Opaque joystick handle */
typedef struct joystick joystick_t;

struct joystick { 
    int fd;
    uint32_t speed_hz;
    int up;
    int down;
    int deadzone; // percent 0 to 100
    bool direction; // 0 = up, 1 = down
    joy_calib_t calib;
};
/* Open joystick SPI device
   dev: device path (use JOY_DEFAULT_DEVICE or NULL for default)
   speed_hz: SPI speed (use JOY_DEFAULT_SPEED_HZ or 0 for default)
   ch: ADC channel numbers (0 or 1)
   Returns pointer to joystick_t on success, NULL on failure.
*/
joystick_t *joy_open(const char *dev, uint32_t speed_hz, int ch_X, int ch_Y);
typedef struct {
    int min_x, max_x;
    int min_y, max_y;
    int center_x, center_y;
    int deadzone_pct;   /* e.g. 5 */
    double trigger_frac;/* e.g. 0.8 = must move 80% towards end */
    int calibrated;     /* 0 = not finalized, 1 = calibrated */
} joy_calib_t;

/* Start calibration: resets min/max to extreme/opposite values */
void joy_calib_start(joystick_t *j);

/* Update calibration with a sample (raw_x, raw_y).
   Should be called repeatedly as you sample ADC while the user moves the stick.
*/
void joy_calib_update(joystick_t *j, int raw_x, int raw_y);

/* Finalize calibration (calculate centers/thresholds). Returns 0 on success. */
int joy_calib_finalize(joystick_t *j);

/* Optional: load/save calibration to file (path). Returns 0 on success. */
int joy_calib_save(joystick_t *j, const char *path);
int joy_calib_load(joystick_t *j, const char *path);

/* Test functions: returns 1 if axis is pressed toward "up" or "down" respectively */
int joy_is_pressed_up(joystick_t *j, int raw_x);
int joy_is_pressed_down(joystick_t *j, int raw_y);