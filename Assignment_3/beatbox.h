#ifndef BEATBOX_H
#define BEATBOX_H

#include <stdio.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

#define BEATBOX_BPM_MIN 40
#define BEATBOX_BPM_MAX 300
#define BEATBOX_BPM_DEFAULT 120

#define BEATBOX_VOLUME_MIN 0
#define BEATBOX_VOLUME_MAX 100
#define BEATBOX_VOLUME_DEFAULT 80

typedef struct {
	int beatSamples;
	short *wavData;
} wavedata_t;

void BeatBoxInit(void);

void BeatBoxCleanup(void);

void BeatBoxReadWavinMem(char *fileName, wavedata_t *wavSound);
    
void BeatBoxFreeWavData(wavedata_t *wavSound);

void BeatBoxNone(void);

void BeatBoxRock(void);

void BeatBoxCustom(int time, int bpm);

// Get/set the volume.
// setVolume() function posted by StackOverflow user "trenki" at:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
int  BeatBoxGetVolume();
void BeatBoxSetVolume(int newVolume);


#endif