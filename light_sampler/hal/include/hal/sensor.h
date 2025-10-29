/**
 * @file sensor.h
 * @brief Sensor header file for reading the light
 * 
 * 
 * *******************
 * 
 * Enabling a PWM configuration:
 * 
 * 1. $ sudo nano /boot/firmware/extlinux/extlinux.conf    # load the correct driver
 * 
 * 2. In the file, enable PWM on GPIO12 (LED Emitter) = HAT pin32 (Without bricking!)
 * 
 *      label microSD (default)
 *      kernel /Image
 *      append console=ttyS2,115200n8 root=/dev/mmcblk1p3 ro ... <trimmed>
 *      fdtdir /
 *      fdt /ti/k3-am67a-beagley-ai.dtb
 *      fdtoverlays /overlays/k3-am67a-beagley-ai-spidev0.dtbo,
 *      fdtoverlays /overlays/k3-am67a-beagley-ai-pwm-epwm0-gpio12.dtbo     # Add this
 *      #initrd /initrd.img                                                 
 * 
 * 3. Reboot the board. After every reboot, you must run this (add to the setup.sh in BYAI):
 * 
 *    $ sudo beagle-pwm-export --pin hat-32
 * 
 * 
 * ********************
 * 
 * Viewing PWM files: 
 * 
 * $ ls /dev/hat/pwm/GPIO12
 * 
 * 
 * *********************
 * 
 * Setting the period of a cycle:
 * 
 * $ cd /dev/hat/pwm/GPIO12/
 * $ echo 0         > duty_cycle
 * $ echo 1000000   > period
 * $ echo 500000    > duty_cycle
 * $ echo 1         > enable
 * 
 * 
 * Constraints:
 *      duty_cycle <= period
 *      0 <= period <= 469,754,879
 * 
 * 
 * Turning off:
 * 
 * $ echo 0 > enable
 * 
 * 
 * **********************
 * 
 * Flash a slow LED:
 * 
 * $ echo 0 > duty_cycle
 * $ echo 250000000 > period
 * $ echo 125000000 > duty_cycle
 * $ echo 1 > enable
 * 
 * 
 * **********************
 * 
 * This link might be useful: https://docs.beagleboard.org/boards/beagley/ai/demos/using-pwm.html
 * 
 * **********************
 * wiring the sensor (from chatGPT cause I cant wire for shit)
 *
 *
 * 3.3V ---/\/\/\---+---/\/\/\--- GND
 *         Sensor   |    10kΩ
 *                  |
 *               ADC CH0 *
 * 
 */

#ifndef _SENSOR_H_
#define _SENSOR_H_

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

// Use the light sensor to read current light level.

// Setup light
int sensor_read();

// Clean up thread
void sensor_cleanup();

// Must be called once every 1s.
// Moves the samples that it has been collecting this second into
// the history, which makes the samples available for reads (below).
void moveCurrentDataToHistory();

// Get the number of samples collected during the previous complete second.
int getHistorySize();

// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `size` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: It provides both data and size to ensure consistency.
double* getHistory(int* size);

// Get the average light level (not tied to the history)
double getAverageReading();

// Get the total number of light level samples taken so far.
long long getNumSamplesTaken();

#endif