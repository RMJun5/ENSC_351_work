/**
 * @file encoder.h
 * @brief Rotary encoder header file for controlling how fast the LED blinks
 */

#ifndef ENCODER_H
#define ENCODER_H

// Use the rotary encoder to control how fast the LED blinks (using PWM).
// This LED emitter is designed to flash directly at the “detector” light sensor.
int read_encoder(void);

// Set the speed of the LED emitter using the rotary encoder.
void set_blink_speed(int speed);


#endif