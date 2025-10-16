#pragma once

#include <stdint.h>

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


/* Open joystick SPI device
   dev: device path (use JOY_DEFAULT_DEVICE or NULL for default)
   speed_hz: SPI speed (use JOY_DEFAULT_SPEED_HZ or 0 for default)
   ch: ADC channel numbers (0 or 1)
   Returns pointer to joystick_t on success, NULL on failure.
*/
joystick_t *joy_open(const char *dev, uint32_t speed_hz, int ch);
