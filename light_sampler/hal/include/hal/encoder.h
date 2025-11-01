/**
 * @file encoder.h
 * @brief Rotary encoder header file for controlling how fast the LED blinks
 */

#ifndef ENCODER_H
#define ENCODER_H


#include <gpiod.h>
#include <stdio.h>

#define CHIPNAME "gpiochip0" // Typically the name of your GPIO chip

enum Direction 
{
    LEFT,
    RIGHT,
    UP,
    DOWN,
    NONE

}


// Use the rotary encoder to control how fast the LED blinks (using PWM).
// This LED emitter is designed to flash directly at the “detector” light sensor.
int read_encoder(void);

#endif