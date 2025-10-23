#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h>

int nanotoms (int ns);

long long runtime(long long present, long long start);
void sleep_ms(long long ms);
    

/* Millisecond wall-clock using CLOCK_REALTIME (not used for intervals in main; kept for reference) */
long long getTimeInMs(void);

