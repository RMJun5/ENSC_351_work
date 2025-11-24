#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H
/**
 * @file accelerometer.h
 * @brief Accelerometer header file for reading X, Y, Z axis data
*/
#include <stdio.h>
#include <stdint.h>


// Initialize the accelerometer
void accelerometer_init(void);
// Read the X-axis data from the accelerometer
int16_t accelerometer_read_x(void);
// Read the Y-axis data from the accelerometer
int16_t accelerometer_read_y(void);
// Read the Z-axis data from the accelerometer
int16_t accelerometer_read_z(void);
//Generate a sound based on the accelerometer data
void accelerometer_generate_sound(int16_t x, int16_t y, int16_t z);
// Thread function to continuously read accelerometer data
void* accelerSoundThread(void* arg);
//cleanup accelerometer resources
void accelerometer_cleanup(void);


#endif// /**
//  * @file accelerometer.h