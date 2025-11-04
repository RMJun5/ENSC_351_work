#ifndef _HELP_H_
#define _HELP_H_
void write2file(const char *path, const char *value);
int devRead(const char *path, uint8_t mode, uint8_t bits, uint32_t speed);
Period_statistics_t printStatistics(enum Period_whichEvent Event);
int read_adc_ch(int fd, int ch, uint32_t speed_hz);
double ADCtoV(int reading);
void sleep_ms(long long ms);
long long getTimeInNanoS(void);
double nanotoms(int ns);
long long getTimeInMs(void);
static void sanitize_line(char *s);
static void to_lower_ascii(char *s);
#endif