#define _GNU_SOURCE

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
#define BUFF_SIZE 2048
// Stay under 1500 bytes (Ethernet MTU). Leave headroom for IP/UDP headers and our text.
#define SEND_CHUNK_LIM 1400

typedef struct { 
    pthread_t ID;
    int socket;
    int packets;
    atomic_bool*shutdown;
    atomic_bool running;
    const struct sockaddr_in *client;
}UDP;

void* udp_listener(void* arg);
int UDP_start(atomic_bool *shutdown_flag);
void UDP_stop(void);
