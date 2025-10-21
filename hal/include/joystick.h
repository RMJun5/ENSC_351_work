#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>


typedef struct joystick joystick_t;

typedef enum {
    DIR_NONE,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} joystick_direction_t;

typedef struct {
    bool calibrated;
} joy_calib_t;

struct joystick { 
    int fd;
    uint32_t speed_hz;
    int up;
    int down;
    int deadzone; // percent 0 to 100
    bool direction; // 0 = up, 1 = down
    int mode;
};


/* Press tests */
int joy_read_ch(int fd, int ch, uint32_t speed_hz);
void initialize_joy(int x, int y);
void calibrate_joy (int fd);
joystick_direction_t joy_get_direction(int x, int y);