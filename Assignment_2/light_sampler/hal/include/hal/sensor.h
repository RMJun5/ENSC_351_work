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
 * 2. In the file, enable PWM on GPIO12 (LED Emitter) = HAT pin32
 * 
 *      label microSD (default)
 *      kernel /Image
 *      append console=ttyS2,115200n8 root=/dev/mmcblk1p3 ro ... <trimmed>
 *      fdtdir /
 *      fdt /ti/k3-am67a-beagley-ai.dtb
 *      fdtoverlays /overlays/k3-am67a-beagley-ai-pwm-epwm0-gpio12.dtbo     # Add this
 *      initrd /initrd.img                                                  # Add this
 * 
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
 *
 * 
 * 
 * 
 */


#ifndef SENSOR_H
#define SENSOR_H

// Use the light sensor to read current light level.
read_sensor();

// Compute and print out the number of dips in the light intensity seen in the previous second
number_of_dips();




#endif