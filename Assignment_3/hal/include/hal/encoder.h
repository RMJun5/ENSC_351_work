// /**
//  * @file encoder.h
//  * @brief Rotary encoder header file for controlling how fast the LED blinks
//  */

#ifndef ENCODER_H
#define ENCODER_H


#include <gpiod.h> // preferably want to try an SPI approach to this instead
#include <stdio.h>

typedef enum 
{
  CW,
  CCW,
  STOPPED,
  PRESSED
} Rotation;

// Use the rotary encoder to control how fast the LED blinks (using PWM).
// This LED emitter is designed to flash directly at the “detector” light sensor.
Rotation read_encoder();
void encoder_init();
void clean_encoder();
Rotation poll_encoder();

#endif