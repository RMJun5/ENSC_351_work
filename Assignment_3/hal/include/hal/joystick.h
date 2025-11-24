#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

typedef enum {
    IDLE,
    JS_UP,
    JS_DOWN,
    JS_LEFT,
    JS_RIGHT
} Direction;

Direction joystick();
Direction getFilteredJoystickInput();

#endif