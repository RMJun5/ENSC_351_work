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
    .clen = 0
};

static pthread_t UDPListenerID;
static bool UDPstarted = false;
static struct sockaddr_in client;

static char last_cmd[64]={0};


// Forward declarations
static void* UDPThread(void* arg);
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

    // 1) Create socket
    udp.sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp.sock < 0) {
        perror("UDP_start: socket");
        return;
    }

    int yes = 1;
    if (setsockopt(udp.sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("UDP_start: setsockopt(SO_REUSEADDR)");
        close(udp.sock);
        udp.sock = -1;
        return;
    }

    // 2) Bind
    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);          // 12345
    addr.sin_addr.s_addr = htonl(INADDR_ANY);    // listen on all local IPs

    if (bind(udp.sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("UDP_start: bind");
        close(udp.sock);
        udp.sock = -1;
        return;
    }

    // 3) Flags
    atomic_store(&udp.shutdown, false);
    atomic_store(&udp.running,  true);
    udp.clen = 0;                                // no client yet

    // 4) Start the listener thread last
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int rc = pthread_create(&UDPListenerID, &attr, UDPThread, NULL);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        fprintf(stderr, "UDP_start: pthread_create failed (%d)\n", rc);
        close(udp.sock);
        udp.sock = -1;
        atomic_store(&udp.running, false);
        return;
    }
}


void UDP_stop(void)
{
    if (!atomic_load(&udp.running)) {
        fprintf(stderr, "UDP_stop: already stopped\n");
        return;
    }

    atomic_store(&udp.shutdown, true);

    // wake select() quickly
    struct sockaddr_in self = {0};
    self.sin_family      = AF_INET;
    self.sin_port        = htons(PORT);
    self.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sendto(udp.sock, "", 0, 0, (struct sockaddr*)&self, sizeof(self));

    pthread_join(UDPListenerID, NULL);

    if (udp.sock >= 0) { close(udp.sock); udp.sock = -1; }
    atomic_store(&udp.running, false);
}


static void send_text(const char *text)
{
    if (udp.clen == 0 || udp.sock < 0 || client.sin_port == 0) return;

    size_t len = strlen(text);
    const char *cursor = text;

    while (len > 0) {
        size_t chunk = (len < CHUNK_LIM) ? len : CHUNK_LIM;
        size_t cut = chunk;

        const char *nl = memrchr(cursor, '\n', chunk);
        if (nl) cut = (size_t)(nl - cursor + 1);

        sendto(udp.sock, cursor, cut, 0,
            (struct sockaddr*)&client, udp.clen);

        cursor += cut;
        len    -= cut;
    }
}


void UDP_help(void)
{
    static const char *help[] = {
        "help - list commands\n",
        "? - same as help\n",
        "count - total samples taken\n",
        "length - samples in last second\n",
        "dips - dips in last second\n",
        "history - 1 value per line (volts)\n",
        "stop - stop server\n",
        "<enter> - repeat last command\n"
    };
    for (size_t i = 0; i < sizeof(help)/sizeof(help[0]); ++i) {
        send_text(help[i]);
    }
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
        // if (last_cmd[0] == '\0') {
        //     UDP_help();
        //     return;
        // }
        // cmd = last_cmd;
        if (last_cmd[0] == '\0') { 
            // First command blank, show help
            cmd = "help";
        } else {
            cmd = last_cmd;  // repeat last command
        }
    } else {
        // Save for blank-repeat
        strncpy(last_cmd, cmd, sizeof(last_cmd)-1);
        last_cmd[sizeof(last_cmd)-1] = '\0';
    }
     if (!strcasecmp(cmd, "help") || !strcmp(cmd, "?")) {
        UDP_help();
    }
    else if (!strcasecmp(cmd, "count")) {
        long long n = sampler_getNumSamplesTaken();
        char buf[128];
        snprintf(buf, sizeof(buf), "# samples taken total: %lld\n", n);
        send_text(buf);
    }
    else if (!strcasecmp(cmd, "length")) {
        int n = sampler_getHistorySize();
        char buf[128];
        snprintf(buf, sizeof(buf), "# samples last second: %d\n", n);
        send_text(buf);
    }
    else if (!strcasecmp(cmd, "dips")) {
        int d = sampler_getHistDips();
        char buf[128];
        snprintf(buf, sizeof(buf), "# dips last second: %d\n", d);
        send_text(buf);
    }
   else if (!strcasecmp(cmd, "history")) {
    int n = 0;
    double *hist = sampler_getHistory(&n);  // malloc'd array of last-second samples
        if (!hist || n <= 0) {
            send_text("History empty\n");
        } else {
            // Use your dedicated formatter that sends 10 values/line, comma-separated, 3 decimals.
            // NOTE: UDP_history(...) frees 'hist' internally, so do NOT free it again here.
            UDP_history(hist, n);
            hist = NULL; // avoid accidental double-free if this code is edited later
        }
    }
    else if (!strcasecmp(cmd, "stop")) {
        send_text("Stopping server...\n");
        atomic_store(&udp.shutdown, true);
    } 
    else {
        send_text("Unknown command. Type 'help'.\n");
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
void* UDPThread(void* arg)
{
    (void)arg;
    char buf[BUFF_SIZE];

    while (!atomic_load(&udp.shutdown)) {
        fd_set rfds;
        FD_ZERO(&rfds);
        if (udp.sock < 0) break;                // safety
        FD_SET(udp.sock, &rfds);
        struct timeval tv = { .tv_sec = 0, .tv_usec = 200000 };
        int r = select(udp.sock + 1, &rfds, NULL, NULL, &tv);
        if (r < 0) { if (errno == EINTR) continue; perror("select"); break; }
        if (r == 0) continue; // timeout

        if (FD_ISSET(udp.sock, &rfds)) {
            socklen_t len = sizeof(client);
            ssize_t n = recvfrom(udp.sock, buf, sizeof(buf)-1, 0,
                                 (struct sockaddr*)&client, &len);
            if (n <= 0) continue;
            buf[n] = '\0';
            trim_line(buf);
            udp.clen = len;                     // remember sender
            dispatch(buf);
        }
    }
    atomic_store(&udp.running, false);
    return NULL;
}
