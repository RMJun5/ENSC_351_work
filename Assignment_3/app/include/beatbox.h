/**
 * @file beatbox.h
 * @brief Contains all the beattypes for the beatbox
 * 
 */

#ifndef BEATBOX_H
#define BEATBOX_H

#include "audioMixer.h"
#include <unistd.h>


typedef struct 
{
  wavedata_t kick;
  wavedata_t snare;
  wavedata_t hihat;
} RockBeat;

typedef struct {
  wavedata_t *sounds;  // dynamically allocated array of sounds
  int numSounds;       // number of sounds in the beat
} CustomBeat;



// // Unused atm
// typedef struct {
//   RockBeat *rockBeats;
//   CustomBeat *customBeats;
// } BeatBox;


typedef enum 
{   
  NONE,
  ROCK,
  CUSTOM
} BeatType;


BeatType BeatBox_init(int beat, RockBeat *data);
void BeatBox_play(RockBeat *beat, int bpm);
void BeatBox_custom();


#endif