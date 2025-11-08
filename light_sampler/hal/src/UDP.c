#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "hal/UDP.h"
#include "hal/sampler.h"
#include "hal/periodTimer.h"
#include "hal/help.h"


#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <ctype.h>  
#include <stdatomic.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>


static UDP udp = {
    .sock = -1,
    .running = ATOMIC_VAR_INIT(false), // active
    .shutdown= ATOMIC_VAR_INIT(false),
    .clen = sizeof(struct sockaddr_in)
};

static pthread_t UDPListenerID;
static bool UDPstarted = false;
static struct sockaddr_in client;
static socklen_t clen = 0;

static char last_cmd[64]={0};


// Forward declarations
// static void* UDPThread(void* arg);
static void dispatch(const char *cmd);
static void send_text(const char *text);



/**
 * @brief Start the UDP server
 * 
 */
void UDP_start(void)
{
    if (atomic_load(&udp.running)) {
        fprintf(stderr, "UDP_start: already running\n");
        return;
    }

    atomic_store(&udp.shutdown, false);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int rc = pthread_create(&UDPListenerID, &attr, UDPThread, NULL);
    pthread_attr_destroy(&attr);

    if (rc != 0) {
        fprintf(stderr, "UDP_start: pthread_create failed (%d)\n", rc);
        return;
    }
}

void UDP_stop(void){
    if (!atomic_load(&udp.running)){
        fprintf(stderr, "UDP_cleanup: UDP server already stopped\n");
        return;
    }
    
    atomic_store(&udp.shutdown, true);

    UDPstarted = false;
    atomic_store_explicit(&udp.shutdown, true, memory_order_release);
    
    // Wake the thread if it's blocked in select: send a dummy packet to self
    // (optional, select() has a timeout above, so this is just a fast-path).
    // You can omit this if you like the 200ms worst-case delay.
    struct sockaddr_in self = {0};
    self.sin_family      = AF_INET;
    self.sin_port        = htons(PORT);
    self.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    (void)sendto(udp.sock, "", 0, 0, (struct sockaddr*)&self, sizeof(self));

    (void)pthread_join(UDPListenerID, NULL);

    close(udp.sock);
    udp.sock = -1;
    UDPstarted = false;
    atomic_store_explicit(&udp.running, false, memory_order_release);
}

void send_text(const char *text ) {

    // Make sure it uses the global socket
    if (clen == 0 || udp.sock < 0) return;
    sendto(udp.sock, text, strlen(text), 0, (struct sockaddr*)&client, clen);

    size_t len = strlen(text);
    const char *cursor = text;
    
    while(len > 0){
        size_t chunk = len < CHUNK_LIM ? len : CHUNK_LIM;
        
        size_t cut = chunk;
        const char *newLine = memrchr(cursor,'\n', chunk);
        if(newLine){
            cut = (size_t)(newLine-cursor+1);
        } else {
            const char *comma = NULL;
            for (size_t i = chunk; i>1;--i){
                if (cursor[i-2]==','&& cursor[i-1]==' '){
                    comma = cursor + (i-1);
                    break;
                }
            }
            if (comma) {
                cut = (size_t)(comma-cursor);
            }
        }
        if (cut == 0 || cut > chunk){
            cut = chunk;
        }
        if (clen == 0 || udp.sock < 0) return;
        (void) sendto(udp.sock,cursor,cut,0,&client , clen);
        cursor += cut;
        len -= cut;
    }
}

void UDP_help (void){
    static const char *help[] = { 
                      "help - Returns a brief summary/list of supported commands.", 
                      "? - Same as help",
                      "count - Return the total number of samples taken",
                      "length - Return the number of samples taken in the previously completed second.",
                      "dips - Return the number of dips in the previously completed second.",
                      "history - Return all the samples in the previously completed second.",
                      "stop - Exit the program.",
                      "<enter>(a blank input) - repeats last command"};
    
    printf("Available commands:\n"); 
    int count = sizeof(help) / sizeof(help[0]);
    for (int i = 0; i < count; ++i) {
        send_text(help[i]);
    }
    // send_text(help);
}

/**
 * @brief Returns a list of commands
 * 
 */
void UDP_question (void){
    //UDP_help(udp.sock, client, clen);
    UDP_help();
}

/**
 * @brief Returns the total number of samples
 * 
 */
void UDP_count(void){
    long long n = sampler_getNumSamplesTaken();
    char countbuf[128];
    int m = snprintf(countbuf,sizeof(countbuf), "# samples taken total: %lld\n", n);
    if (m) send_text(countbuf);
}
void UDP_length(void){
    int size = sampler_getHistorySize();
    char lengthbuf[128];
    int m = snprintf(lengthbuf, sizeof(lengthbuf), "# of samples taken last second: %d \n", size);
    if (m) {send_text(lengthbuf);}
}

/**
 * @brief Returns the number of dips in the last second
 * 
 */
void UDP_dips(void){
    int d = sampler_getHistDips();
    char dipbuf[128];
    int m = snprintf(dipbuf, sizeof(dipbuf), "# of dips: %d \n", d);
    if (m) {
        // send_text(udp.sock, client,clen, dipbuf);
        send_text(dipbuf);
    }
}

/**
 * @brief Sends the last-second history as voltages, 10 values per line, 3 decimals.
 * Format lines containing up to 10 values and send each line as a
 * separate message. This guarantees no single sample's digits are split
 * across packets and keeps each UDP packet well under the MTU.
 *
 * @param hist the history
 * @param n the number of samples
 */
void UDP_history(double *hist, int n) { // get rid of n param
    
    if (!hist|| n <= 0) {
        const char *none;
        if (n == 0) {
            none = "History empty\n";
        }
        else {
            none = "History unavailable (first sample)\n";
        }
        send_text(none);
        free((void*)hist);
        return;
    }

    n = sampler_getHistorySize(); // or make this a diff variable
    if (n <= 0) {
        const char *none = "History empty\n";
        send_text(none);
        free(hist);
        return;
    }

    size_t line_cap = CHUNK_LIM;
    char *line = malloc(line_cap + 1);
    if (!line) {
        const char *err = "Server out of memory\n";
        send_text(err);
        free((void*)hist);
        return;
    }

    int count_in_line = 0;
    size_t off = 0;

    for (int i = 0; i < n; ++i) {
        double volts = ADCtoV((int)hist[i]);

        /* Format the sample to 3 decimal places */
        int wrote = snprintf(line + off, line_cap - off + 1, "%.3f", volts);
        if (wrote < 0) wrote = 0;
        off += (size_t)wrote;

        ++count_in_line;

        /* Decide separator: comma+space or newline (10 numbers per line) */
        if (count_in_line >= 10 || i == n - 1) {
            /* end the current line with newline */
            if (off < line_cap) line[off++] = '\n';
            line[off] = '\0';
            send_text(line);

            /* reset for next line */
            off = 0;
            count_in_line = 0;
        } else {
            /* add comma + space */
            if (off + 2 < line_cap) {
                line[off++] = ',';
                line[off++] = ' ';
                line[off] = '\0';
            } else {
                /* If somehow line would overflow, flush what we have now
                 * (this keeps individual samples intact).
                 */
                line[off] = '\0';
                send_text(line);
                off = 0;
            }
        }
    }

    free(line);
    free((void*)hist);
    return;
}

/**
 * @brief Dispatch a command to the terminal 
 * 
 * @param cmd the typed command
 */
static void dispatch(const char *cmd) {   
    if(cmd[0] == '\0'){
        // Repeat last command
        if (last_cmd[0] == '\0') {
            UDP_help();
            return;
        }
        cmd = last_cmd;
    } else {
        // Save for blank-repeat
        strncpy(last_cmd, cmd, sizeof(last_cmd)-1);
        last_cmd[sizeof(last_cmd)-1] = '\0';
    } 
    if (!strcasecmp(cmd, "help") || !strcmp(cmd, "?")) {
        UDP_help();
    } 
    else if (!strcasecmp(cmd, "count")) {
        UDP_count();
    } 
    else if (!strcasecmp(cmd, "length")) {
        UDP_length();
    } 
    else if (!strcasecmp(cmd, "dips")) {
        UDP_dips();
    }
    else if (!strcasecmp(cmd, "history")) {
        int n = 0;
        double *hist = sampler_getHistory(&n);  // malloc'd copy + count
        UDP_history(hist, n);
    } 
    else if (!strcasecmp(cmd, "stop")) {
        const char *m = "Stopping server...\n";
        send_text(m);
        if (udp.shutdown) {
            atomic_store_explicit(&udp.shutdown, true, memory_order_release);
        }
    } 
    else {
        const char *m = "Unknown command. Type 'help'.\n";
        send_text(m);
    }
}

/**
 * @brief Trim whitespace from the end of a line
 * 
 * @param line the line of text to trim
 */
void trim_line(char *line) {
    int len = strlen(line);
    while (len > 0 && isspace(line[len-1])) {
        line[--len] = '\0';
    }
}

/**
 * @brief The UDP server thread
 * 
 * @param arg 
 * @return void* 
 */
void* UDPThread(void* arg) {
    (void)arg;
    if (udp.sock < 0) {
        int s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0) {
            perror("UDPThread: socket");
            return NULL;
        }
    
    int yes = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
            perror("UDPThread: setsockopt(SO_REUSEADDR)");
            close(s);
            return NULL;
        }
    struct sockaddr_in addr = {0};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
          if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("UDPThread: bind");
            close(s);
            return NULL;
        }

        udp.sock = s;
    }
    atomic_store(&udp.running, true);
    atomic_store(&udp.shutdown, false);
    char buf[BUFF_SIZE];
    // while (!udp.shutdown || !atomic_load_explicit(udp.shutdown, memory_order_acquire)) {
    while (!atomic_load_explicit(&udp.shutdown, memory_order_acquire)) {

        // Wait for data or wake periodically to check shutdown
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(udp.sock, &rfds);
        struct timeval tv = { .tv_sec = 0, .tv_usec = 200000 }; // 200 ms
        int r = select(udp.sock + 1, &rfds, NULL, NULL, &tv);

        if (r < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        } else if (r == 0) {
            // timeout, loop to check shutdown
            continue;
        }

        if (FD_ISSET(udp.sock, &rfds)) {
            ssize_t n = recvfrom(udp.sock, buf, sizeof(buf)-1, 0,
                                 (struct sockaddr*)&client, &clen);
            if (n < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;
                perror("recvfrom");
                break;
            }
            buf[n] = '\0';
            trim_line(buf);
            dispatch(buf);
        }
    }
    atomic_store_explicit(&udp.running, false, memory_order_release);
    return NULL;

}