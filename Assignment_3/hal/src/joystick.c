#include "hal/joystick.h"


// Channels for joystick UP and DOWN:
int CH0; // X
int CH1; // Y

// MCP3208 ADC values:
const char* dev; // path to spidev
uint8_t mode; // SPI mode 0
uint8_t bits; // bits per w
uint32_t speed; // clk speed

static int read_adc_ch(int fd, int ch, uint32_t speed_hz) {

    uint8_t tx[3] = {   (uint8_t)(0x06 | ((ch & 0x04) >> 2)),
                    (uint8_t)((ch & 0x03) << 6),
                    0x00
                };
    
    uint8_t rx[3] = { 0 };
            
    // Struct that sends the transfer:
    struct spi_ioc_transfer tr = 
    {
    .tx_buf = (unsigned long)tx,
    .rx_buf = (unsigned long)rx,
    .len = 3,
    .speed_hz = speed_hz,
    .bits_per_word = 8,
    .cs_change = 0
    };

    // Ask the kernel to send SPI message and get reply, otherwise fail
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) return -1;

    // Return result into recieved data:
    return ((rx[1] & 0x0F) << 8) | rx[2]; // 12-bits


}

Direction joystick() {

    dev = "/dev/spidev0.0"; // path to spidev
    mode = 0; // SPI mode 0
    bits = 8; // bits per w
    speed = 250000; // clk speed

    int fd = open(dev, O_RDWR);

    if (fd < 0) { perror("open"); return IDLE; }
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) { perror("mode"); return IDLE; }
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) { perror("bpw"); return IDLE; }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) { perror("speed"); return IDLE; }

    CH0 = read_adc_ch(fd, 0, speed);
    CH1 = read_adc_ch(fd, 1, speed);

    //printf("CH0=%d CH1=%d \n", CH0, CH1);
    close(fd);

    if (CH1 < 600 && CH0 > 500 && CH0 < 3500) return JS_DOWN;
    else if (CH1 > 2600 && CH0 > 500 && CH0 < 3500) return JS_UP;
    else if (CH0 < 600 && CH1 > 500 && CH1 < 3500) return JS_LEFT;
    else if (CH0 > 2600 && CH1 > 500 && CH1 < 3500) return JS_RIGHT;
    else return IDLE;

}
