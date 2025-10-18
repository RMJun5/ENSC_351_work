#define _POSIX_C_SOURCE 200809L 
#include "joystick.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <limits.h>
#include <assert.h>
#include <math.h>

static bool joystick_initialized = false;
static joystick_t *g_joy = NULL;

joystick_t *joy_open(const char *dev, uint32_t speed_hz, int mode, int bits, int ch_X, int ch_Y) {
    joystick_t *joy = malloc(sizeof(joystick_t));
    if (!joy) { perror("malloc"); return NULL; }

    joy->fd = open(dev, O_RDWR);
    if (joy->fd < 0) { perror(dev); free(joy); return NULL; }

    joy->speed_hz = speed_hz;
    joy->up = ch_X;
    joy->down = ch_Y;
    joy->deadzone = 10;
    joy->direction = 0;
    joy->mode = mode;

    if (ioctl(joy->fd, SPI_IOC_WR_MODE, &mode) == -1 ||
        ioctl(joy->fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1 ||
        ioctl(joy->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) == -1) {
        perror("SPI config"); close(joy->fd); free(joy); return NULL;
    }

    return joy;
}

int joy_read(int fd, int ch, uint32_t speed_hz) {
    if (ch < 0 || ch > 7) return -1;

    uint8_t tx[3] = { (uint8_t)(0x06 | ((ch & 0x04) >> 2)),
                       (uint8_t)((ch & 0x03) << 6),
                       0x00 };
    uint8_t rx[3] = {0};
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed_hz,
        .bits_per_word = 8
    };
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) return -1;

    return ((rx[1] & 0x0F) << 8) | rx[2];
}

int joy_read_raw(joystick_t *j, int ch) {
    if (!j) return -1;
    return joy_read(j->fd, ch, j->speed_hz);
}

void joy_close(joystick_t *j) {
    if (!j) return;
    if (j->fd >= 0) close(j->fd);
    free(j);
}

bool joystick_init(const char *dev, uint32_t speed_hz, int mode, int bits, int ch_X, int ch_Y) {
    g_joy = joy_open(dev, speed_hz, mode, bits, ch_X, ch_Y);
    if (!g_joy) return false;
    joystick_initialized = true;
    return true;
}

void joystick_cleanup(void) {
    if (g_joy) { joy_close(g_joy); g_joy = NULL; }
    joystick_initialized = false;
}
/* Returns 1 if joystick is pushed "up" (positive Y) */
int joy_is_pressed_up(joystick_t *j, int raw_y) {
    if (!j) return 0;
    int half_range = j->calib.max_y - (1 << (JOY_ADC_BITS - 1));
    if (half_range <= 0) return 0;
    double threshold = (1 << (JOY_ADC_BITS - 1)) + j->calib.trigger_frac * half_range;
    return (raw_y >= (int)threshold) ? 1 : 0;
}

/* Returns 1 if joystick is pushed "down" (negative Y) */
int joy_is_pressed_down(joystick_t *j, int raw_y) {
    if (!j) return 0;
    int half_range = (1 << (JOY_ADC_BITS - 1)) - j->calib.min_y;
    if (half_range <= 0) return 0;
    double threshold = (1 << (JOY_ADC_BITS - 1)) - j->calib.trigger_frac * half_range;
    return (raw_y <= (int)threshold) ? 1 : 0;
}



joystick_direction_t joystick_get_direction(void) {
    assert(joystick_initialized);
    int raw_x = joy_read_raw(g_joy, JOY_DEFAULT_AXIS_X);
    int raw_y = joy_read_raw(g_joy, JOY_DEFAULT_AXIS_Y);
    if (raw_x < 0 || raw_y < 0) return DIR_NONE;
    if (joy_is_pressed_up(g_joy, raw_x)==1) return DIR_UP;
    if (joy_is_pressed_down(g_joy, raw_y)==1) return DIR_DOWN;
    return DIR_NONE;
}
