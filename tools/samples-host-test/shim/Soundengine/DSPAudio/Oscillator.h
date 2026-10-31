#pragma once
#include <stdint.h>
#include "lxr0.h"
#include "Soundengine/config.h"
typedef struct { int16_t output; uint32_t phaseInc,phase; float freq; uint8_t waveform; float pitchMod,fmMod,modNodeValue,cvFreq; uint32_t sampleHi; } OscInfo;
void calcUserSampleOscFmBlock(OscInfo*,int16_t*,int16_t*,uint8_t,const float);
void calcUserSampleOscBlock(OscInfo*,int16_t*,const uint8_t,const float);
void osc_calcUserSampleFreq(OscInfo*);
