/**
 * @file beatbox.h
 * @brief Contains all the beattypes for the beatbox
 * 
 */

#ifndef BEATBOX_H
#define BEATBOX_H

#include "audioMixer.h"
#include <unistd.h>

typedef enum 
{   
  NONE,
  ROCK,
  CUSTOM
} BeatType;


typedef struct {
    wavedata_t kick;
    wavedata_t snare;
    wavedata_t hihat;
} RockBeat;

typedef struct {
    wavedata_t kick;
    wavedata_t snare;
    wavedata_t hihat;
} CustomBeat;

typedef struct {
    int none;
} NoBeat;


// unified BeatBox struct
typedef struct 
{
  BeatType type;
  union 
  {
    RockBeat rock;
    CustomBeat custom;
    NoBeat noBeat;
  } data;
} BeatBox;


void BeatBox_init(BeatBox *beatBox);
void BeatBox_playRock(BeatBox *beatBox, int bpm);
void BeatBox_playCustom(BeatBox *beatBox, int bpm);
void BeatBox_cleanup(BeatBox *beatBox);

#endif