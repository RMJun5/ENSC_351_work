#ifndef _UDP_H_
#define _UDP_H_
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
#include <stdbool.h>

#define PORT 12345
#define BUFF_SIZE 2048
// Stay under 1500 bytes (Ethernet MTU). Leave headroom for IP/UDP headers and our text.
#define CHUNK_LIM 1400
#define SEND_CHUNK_LIM 1400

typedef struct {
    int sock;
    atomic_bool*shutdown;
    volatile bool running;
}UDP;

void* udp_listener(void* arg);
int UDP_start(atomic_bool *shutdown_flag);
void UDP_stop(void);
void send_text(int socket, const struct sockaddr_in *client, socklen_t clen, const char *text );
#endif