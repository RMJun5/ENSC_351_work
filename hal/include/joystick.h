#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h> // for abs

#define JOY_DEFAULT_DEVICE    "/dev/spidev0.0"
#define JOY_DEFAULT_SPEED_HZ  250000U
#define JOY_DEFAULT_AXIS_X    0
#define JOY_DEFAULT_AXIS_Y    1
#define JOY_ADC_BITS          8

typedef struct joystick joystick_t;

typedef enum {
    DIR_NONE,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} joystick_direction_t;

typedef struct {
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int center_x;
    int center_y;
    int deadzone_pct;
    double trigger_frac;
    int calibrated;
} joy_calib_t;

struct joystick { 
    int fd;
    uint32_t speed_hz;
    int up;
    int down;
    int deadzone; // percent 0 to 100
    bool direction; // 0 = up, 1 = down
    int mode;
    joy_calib_t calib;
};

/* Core joystick functions */
joystick_t *joy_open(const char *dev, uint32_t speed_hz, int mode, int bit, int ch_X, int ch_Y);
void joy_close(joystick_t *j);
int joy_read_raw(joystick_t *j, int axis);

/* Modular helpers */
bool joystick_init(const char *device, uint32_t speed_hz, int mode, int bits, int ch0, int ch1);
void joystick_cleanup(void);
/* Press tests */
int joy_is_pressed_up(joystick_t *j, int raw_y);
int joy_is_pressed_down(joystick_t *j, int raw_y);
joystick_direction_t joystick_get_direction(void);

/* Calibration helpers */
void joy_calib_start(joystick_t *j);
void joy_calib_update(joystick_t *j, int raw_x, int raw_y);
int joy_calib_finalize(joystick_t *j);
int joy_calib_save(joystick_t *j, const char *path);
int joy_calib_load(joystick_t *j, const char *path);