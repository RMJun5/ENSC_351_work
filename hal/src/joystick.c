#define _POSIX_C_SOURCE 200809L
#include "joystick.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <errno.h>
#include <math.h>

joystick_t *joy_open(const char *dev, uint32_t speed_hz, int ch_X, int ch_Y) {
    joystick_t *joy = malloc(sizeof(joystick_t));
    if (!joy) {
        perror("malloc");
        return NULL;
    }

    joy->fd = open(dev, O_RDWR);
    if (joy->fd < 0) {
        perror(dev);
        free(joy);
        return NULL;
    }
    joy->speed_hz = speed_hz;
    joy->up = ch_X;
    joy->down = ch_Y;
    joy->deadzone = 10; // default deadzone
    joy->direction = 0;  // default direction

    return joy;
    if (ioctl(joy->fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("SPI_IOC_WR_MODE"); close(fd); return NULL;
    }
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("SPI_IOC_WR_BITS_PER_WORD"); close(fd); return NULL;
    }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) == -1) {
        perror("SPI_IOC_WR_MAX_SPEED_HZ"); close(fd); return NULL;
    }
    return joy;
}

joystick_t *joy_read (int fd, int ch, uint32_t speed_hz){
    if (ch<0 || ch>7) return 1; // invalid channel 
    uint8_t tx[3] = { (uint8_t)(0x06 | ((ch & 0x04) >> 2)), (uint8_t)((ch & 0x03) << 6), 0x00 };
    uint8_t rx[3] = {0};   
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed_hz,
        .bits_per_word = 8,
        .cs_change = 0
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) return -1;
    int val =  ((rx[1] & 0x0F) << 8) | rx[2]; // 12-bit result
    return val;
}
#define JOY_CALIBRATION_DISABLED
#ifdef JOY_CALIBRATION_DISABLED
/* Start / reset calibration */
void joy_calib_start(joystick_t *j) {
    if (!j) return;
    j->calib.min_x = INT_MAX;
    j->calib.max_x = INT_MIN;
    j->calib.min_y = INT_MAX;
    j->calib.max_y = INT_MIN;
    j->calib.center_x = (1 << (JOY_ADC_BITS - 1));
    j->calib.center_y = (1 << (JOY_ADC_BITS - 1));
    j->calib.deadzone_pct = 5;
    j->calib.trigger_frac = 0.80;
    j->calib.calibrated = 0;
}

/* Update min/max with a new raw sample */
void joy_calib_update(joystick_t *j, int raw_x, int raw_y) {
    if (!j) return;
    if (raw_x < j->calib.min_x) j->calib.min_x = raw_x;
    if (raw_x > j->calib.max_x) j->calib.max_x = raw_x;
    if (raw_y < j->calib.min_y) j->calib.min_y = raw_y;
    if (raw_y > j->calib.max_y) j->calib.max_y = raw_y;
}

/* Finalize: compute centers and sanity-check ranges */
int joy_calib_finalize(joystick_t *j) {
    if (!j) return -1;
    /* clamp min/max to ADC bounds */
    int adc_max = (1 << JOY_ADC_BITS) - 1;
    if (j->calib.min_x < 0) j->calib.min_x = 0;
    if (j->calib.min_y < 0) j->calib.min_y = 0;
    if (j->calib.max_x > adc_max) j->calib.max_x = adc_max;
    if (j->calib.max_y > adc_max) j->calib.max_y = adc_max;

    /* fallback if min/max never changed */
    if (j->calib.min_x == INT_MAX || j->calib.max_x == INT_MIN) {
        j->calib.min_x = 0;
        j->calib.max_x = adc_max;
    }
    if (j->calib.min_y == INT_MAX || j->calib.max_y == INT_MIN) {
        j->calib.min_y = 0;
        j->calib.max_y = adc_max;
    }

    j->calib.center_x = (j->calib.min_x + j->calib.max_x) / 2;
    j->calib.center_y = (j->calib.min_y + j->calib.max_y) / 2;
    j->calib.calibrated = 1;
    return 0;
}

/* Save calibration to a file (simple text): min_x max_x min_y max_y center_x center_y deadzone trigger_frac */
int joy_calib_save(joystick_t *j, const char *path) {
    if (!j || !path) return -1;
    FILE *f = fopen(path, "w");
    if (!f) {
        perror("joy_calib_save fopen");
        return -1;
    }
    int r = fprintf(f, "%d %d %d %d %d %d %d %g\n",
        j->calib.min_x, j->calib.max_x, j->calib.min_y, j->calib.max_y,
        j->calib.center_x, j->calib.center_y, j->calib.deadzone_pct,
        j->calib.trigger_frac);
    fclose(f);
    return (r > 0) ? 0 : -1;
}

/* Load calibration; returns 0 on success */
int joy_calib_load(joystick_t *j, const char *path) {
    if (!j || !path) return -1;
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int minx, maxx, miny, maxy, cx, cy, dz;
    double tf;
    int r = fscanf(f, "%d %d %d %d %d %d %d %lf", &minx, &maxx, &miny, &maxy, &cx, &cy, &dz, &tf);
    fclose(f);
    if (r == 8) {
        j->calib.min_x = minx; j->calib.max_x = maxx;
        j->calib.min_y = miny; j->calib.max_y = maxy;
        j->calib.center_x = cx; j->calib.center_y = cy;
        j->calib.deadzone_pct = dz;
        j->calib.trigger_frac = tf;
        j->calib.calibrated = 1;
        return 0;
    }
    return -1;
}
#endif /* JOY_CALIBRATION_ENABLED */

/* Helper: determine if raw_x is pressed "up" (i.e., pushed upward strongly)
   We assume "up" maps to decreasing raw_x (depends on wiring; adjust if necessary).
*/
int joy_is_pressed_up(joystick_t *j, int raw_x) {
    if (!j || !j->calib.calibrated) return 0;
    double half_range = (j->calib.center_x - j->calib.min_x);
    if (half_range <= 0) return 0;
    double threshold = j->calib.center_x - j->calib.trigger_frac * half_range;
    /* require raw_x <= threshold */
    return (raw_x <= (int)threshold) ? 1 : 0;
}

/* Helper: determine if raw_y is pressed "down" (pushed downward strongly)
   Here we assume down maps to increasing raw_y.
*/
int joy_is_pressed_down(joystick_t *j, int raw_y) {
    if (!j || !j->calib.calibrated) return 0;
    double half_range = (j->calib.max_y - j->calib.center_y);
    if (half_range <= 0) return 0;
    double threshold = j->calib.center_y + j->calib.trigger_frac * half_range;
    return (raw_y >= (int)threshold) ? 1 : 0;
}
/* Close joystick */
void joy_close(joystick_t *j) {
    if (!j) return;
    if (j->fd >= 0) close(j->fd);
    free(j);
}
