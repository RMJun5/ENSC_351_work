#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUFF_SIZE 1024

typedef struct { 
    pthread_t ID;
    int packets;
    bool running;
}UDP;

void* udp_listener(void* arg);
int UDP_start(atomic_bool *shutdown_flag);
void UDP_stop(void);
