/*
 * lxr0_reverbBus.c - lxr-0: parallel global reverb bus (see lxr0_reverbBus.h).
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "Soundengine/DSPAudio/FX/lxr0_reverbBus.h"

#if LXR0_REVERB
#include <string.h>
#include "stm32f7xx_hal.h"
#include "Soundengine/DSPAudio/FX/Reverb.h"
#include "Soundengine/DSPAudio/mixer.h"
#include "Soundengine/config.h"
#include "menu/menu.h"
#include "Parameters.h"

#define VERB_BUFFER_SIZE (14*MAX_ALLPASS_SIZE)	// Reverb_init (SPIN_SEMI 0) uses 14 lines of MAX_ALLPASS_SIZE
#define NUM_VERB_PARAMS (PAR_LXR0_VERB_REFLECTION - PAR_LXR0_VERB_SEND1 + 1)
#define LOAD_OFF 95		// bypass when blocks take more than this % of the block time...
#define OFF_BLOCKS 8	// ...this many in a row, so one late block doesn't cut the reverb
#define LOAD_ON 90		// back on when load + the reverb's own cost stays below this %...
#define ON_BLOCKS 345	// ...for 0.5 s (64-sample blocks at 44.1 kHz)

static __fp16 verbBuffer[VERB_BUFFER_SIZE];	// .bss in SRAM1; FX_BUFFER (DTCM) stays the FX bus's
static Reverb verb;
static int16_t sendSum[OUTPUT_DMA_SIZE*2];	// interleaved L/R, Reverb_process mixes them to mono
static int16_t wet[OUTPUT_DMA_SIZE*2];
static float sendGain[6];
static float amount;
static uint8_t routing;
static uint8_t lastParams[NUM_VERB_PARAMS];
static uint32_t blockStart;
static uint32_t budget;		// CPU cycles per block
static uint32_t onCount;
static uint32_t overCount;
static float verbCost;		// % of the block the reverb itself takes, averaged. Held while bypassed
static uint8_t peakLoad;

volatile uint8_t lxr0Verb_load;
volatile uint8_t lxr0Verb_bypassed;

void lxr0Verb_init(void)
{
	verb.reverbBufferSram = verbBuffer;
	verb.stereo = 1;
	Reverb_init(&verb);
	budget = (uint32_t)(((uint64_t)SystemCoreClock * OUTPUT_DMA_SIZE) / 44100);
	memset(lastParams, 0xff, sizeof(lastParams)); // apply every setting on the first block
}

// settings are kit parameters above 255, which are not sent to the engine: poll them
static void applyParams(void)
{
	const uint8_t* p = &activeParameters[PAR_LXR0_VERB_SEND1];
	for(uint8_t i=0; i<NUM_VERB_PARAMS; i++)
	{
		if(p[i] == lastParams[i])
			continue;
		lastParams[i] = p[i];

		const float v = p[i]/127.f;
		switch(PAR_LXR0_VERB_SEND1 + i)
		{
		case PAR_LXR0_VERB_AMOUNT:		amount = v;						break;
		case PAR_LXR0_VERB_ROUTING:		routing = p[i];					break;
		// as FxBus_updateParameter for FX_REVERB (P1-P4)
		case PAR_LXR0_VERB_PREDELAY:	Reverb_setDelayTime(&verb, v);	break;
		case PAR_LXR0_VERB_DECAY:		Reverb_setFeedback(&verb, v);	break;
		case PAR_LXR0_VERB_TONE:		Reverb_setComposition(&verb, v);break;
		case PAR_LXR0_VERB_REFLECTION:	Reverb_setVariation(&verb, v);	break;
		default:						sendGain[i] = v;				break; // sends 1-6
		}
	}
}

void lxr0Verb_beginBlock(void)
{
	blockStart = DWT->CYCCNT;
	applyParams();
	memset(sendSum, 0, sizeof(sendSum));
}

void lxr0Verb_addSend(uint8_t voice, const int16_t* data)
{
	const float g = sendGain[voice];
	if(g <= 0.f || lxr0Verb_bypassed)
		return;

	int32_t* sum = (int32_t*)sendSum;
	for(uint8_t i=0; i<OUTPUT_DMA_SIZE; i++)
	{
		const int16_t s = (int16_t)(data[i] * g);
		sum[i] = __QADD16(sum[i], __PKHBT(s, s, 16));
	}
}

// add the scaled wet signal to the output chosen like FxBus_process does
static void mixWet(int16_t* output, int16_t* output2)
{
	int16_t* out;
	uint8_t stereo = 0;
	switch(routing)
	{
	default:
	case MIXER_ROUTING_DAC1_STEREO:	out = output2;		stereo = 1;	break;
	case MIXER_ROUTING_DAC2_STEREO:	out = output;		stereo = 1;	break;
	case MIXER_ROUTING_DAC1_L:		out = output2;					break;
	case MIXER_ROUTING_DAC1_R:		out = output2+1;				break;
	case MIXER_ROUTING_DAC2_L:		out = output;					break;
	case MIXER_ROUTING_DAC2_R:		out = output+1;					break;
	}

	for(uint8_t i=0; i<OUTPUT_DMA_SIZE; i++)
	{
		const int16_t l = (int16_t)(wet[2*i] * amount);
		const int16_t r = (int16_t)(wet[2*i+1] * amount);
		if(stereo)
		{
			int32_t* o = (int32_t*)&out[2*i];
			*o = __QADD16(*o, __PKHBT(l, r, 16));
		}
		else
		{
			out[2*i] = (int16_t)__SSAT(out[2*i] + (l + r)/2, 16); // mono out: sum of both sides
		}
	}
}

void lxr0Verb_endBlock(int16_t* output, int16_t* output2)
{
	uint32_t verbCycles = 0;
	if(!lxr0Verb_bypassed && amount > 0.f)
	{
		const uint32_t t0 = DWT->CYCCNT;
		memset(wet, 0, sizeof(wet));
		Reverb_process(&verb, sendSum, wet, &wet[1], OUTPUT_DMA_SIZE, 2, 1.f); // all wet
		mixWet(output, output2);
		verbCycles = DWT->CYCCNT - t0;
	}

	const uint32_t load = (DWT->CYCCNT - blockStart) * 100 / budget;
	lxr0Verb_load = load > 255 ? 255 : (uint8_t)load;
	if(lxr0Verb_load > peakLoad)
		peakLoad = lxr0Verb_load;

	if(verbCycles)
	{
		const float cost = verbCycles * 100.f / budget;
		verbCost = verbCost == 0.f ? cost : verbCost + (cost - verbCost)/64.f;
	}

	if(!lxr0Verb_bypassed)
	{
		overCount = load > LOAD_OFF ? overCount + 1 : 0;
		if(overCount >= OFF_BLOCKS)
		{
			lxr0Verb_bypassed = 1;
			onCount = 0;
			overCount = 0;
		}
	}
	else if(load + verbCost < LOAD_ON)
	{
		if(++onCount >= ON_BLOCKS)
		{
			memset(verbBuffer, 0, sizeof(verbBuffer)); // don't replay the old tail
			lxr0Verb_bypassed = 0;
		}
	}
	else
	{
		onCount = 0;
	}
}

uint8_t lxr0Verb_takePeakLoad(void)
{
	const uint8_t peak = peakLoad;
	peakLoad = lxr0Verb_load;
	return peak;
}

uint8_t lxr0Verb_ownLoad(void)
{
	return verbCost > 255.f ? 255 : (uint8_t)verbCost;
}

void lxr0Verb_kitDefaults(uint8_t* para, uint32_t bytesRead)
{
	if(bytesRead > PAR_LXR0_VERB_SEND1)
		return; // the kit has the reverb tail; sends stay 0 otherwise
	para[PAR_LXR0_VERB_AMOUNT] = 100;
	para[PAR_LXR0_VERB_ROUTING] = 0;	// St1, as the FX bus default
	para[PAR_LXR0_VERB_PREDELAY] = 10;
	para[PAR_LXR0_VERB_DECAY] = 90;
	para[PAR_LXR0_VERB_TONE] = 64;
	para[PAR_LXR0_VERB_REFLECTION] = 40;
}
#endif
