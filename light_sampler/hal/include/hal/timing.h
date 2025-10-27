#define _POSIX_C_SOURCE 200809L 
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

double nanotoms (int ns);


long long getTimeInMs(void);

void sleep_ms(long long ms);