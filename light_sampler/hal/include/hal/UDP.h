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
    atomic_bool shutdown;
    atomic_bool running;
}UDP;

void* UDPThread(void* arg);

void UDP_help (void);
void UDP_question (void);
void UDP_count(void);
void UDP_length(void);
void UDP_dips(void);   
void UDP_history(double* hist, int n);

void UDP_start(void);
void UDP_stop(void);
void send_text(const char *text );
#endif