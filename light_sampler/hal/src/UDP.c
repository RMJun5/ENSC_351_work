#define _POSIX_C_SOURCE 200809L
#include "hal/UDP.h"
#include "hal/sampler.h"
#include "periodTimer.h"
#include "hal/help.h"


#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>


static UDP udp = {
    .running = ATOMIC_VAR_INIT(false),
    .sock = -1,
    .shutdown=NULL,
};
static pthread_t UDPlistener;

static char last_cmd[64]={0};

void send_text(int socket, const struct sockaddr_in *client, socklen_t clen, const char *text ){
    size_t len = strlen(text);

    const char *cursor = text;
    while(len > 0){
        size_t chunk = len < SEND_CHUNK_LIM ? len : SEND_CHUNK_LIM;
        
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
        (void) sendto(sock,cursor,cut,0,(const struct sockaddr*)client, clen);
        cursor += cut;
        len -= cut;
    }
}

void UDP_help (int sock, const struct sockaddr_in *client, socklen_t clen){
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
    send_text(sock, client, clen, help);
}
void UDP_question (int sock, const struct sockaddr_in *client,socklen_t clen){
    UDP_help(sock, client, clen);
}
void UDP_count(int sock, const struct sockaddr_in *client,socklen_t clen){
    long long n = sampler_getNumSamplesTaken();
    char countbuf[128];
    int m = snprintf(countbuf,sizeof(countbuf), "# samples taken total: %lld\n", n);
    if (m) send_text(sock, client, clen, countbuf);
}
void UDP_length(int sock, const struct sockaddr_in *client,socklen_t clen){
    int n = sampler_getHistorySize();
    char lengthbuf[128];
    int m = snprintf(lengthbuf, sizeof(lengthbuf), "# of samples taken last second: %d \n", n);
    if(m) send_text(sock, client, clen, lengthbuf);
}
void UDP_dips(int sock, const struct sockaddr_in *client,socklen_t clen){
    int d = sampler_getHistDips();
    char dipbuf[128];
    int m = snprintf(dipbuf, sizeof(dipbuf), "# of samples taken last second: %d \n", d);
    if (m) send_text(sock, client,clen, dipbuf)
}
void UDP_history(int sock, const struct sockaddr_in *client, double* hist, socklen_t clen) {
    /* Send the last-second history as voltages, 10 values per line, 3 decimals.
     * Format lines containing up to 10 values and send each line as a
     * separate message. This guarantees no single sample's digits are split
     * across packets and keeps each UDP packet well under the MTU.
     */
    if (!hist) {
        const char *none = "History unavailable (first sample)\n";
        send_text(sock, client, clen, none);
        return;
    }

    int n = sampler_getHistorySize();
    if (n <= 0) {
        const char *none = "History empty\n";
        send_text(sock, client, clen, none);
        free(hist);
        return;
    }

    size_t line_cap = CHUNK_LIM;
    char *line = malloc(line_cap + 1);
    if (!line) {
        const char *err = "Server out of memory\n";
        send_text(sock, client, clen, err);
        free(hist);
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
            send_text(sock, client, clen, line);

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
                send_text(sock, client, clen, line);
                off = 0;
            }
        }
    }

    free(line);
    free(hist);
}