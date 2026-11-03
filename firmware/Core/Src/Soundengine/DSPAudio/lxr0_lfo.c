/*
 * lxr0_lfo.c - lxr-0: LFO shapes after the vendor's eight. Same 0-1 output, rate, retrigger voice and
 * offset knobs as the vendor shapes; new values of the LFO waveform parameter, so kits need no new bytes.
 *
 * nse  smooth random: a Catmull-Rom spline through a new random value each cycle, so it moves through
 *      each value without stopping. It can overshoot a little at a turn and is clipped to 0-1.
 *      A retrigger starts a new stretch from where it is, with the same slope: no jump.
 * xtr  exponential triangle: rises on xup's cubic to the top at half a cycle, falls on xdn's.
 * One-shots (si1 tr1 su1 sd1 sq1 xu1 xd1 xt1): each hit of the retrigger voice runs one cycle of the
 *      shape, then holds its last value. sq1 is on for the first half, then off. Offset is a delay
 *      before the shot fires, up to one cycle. With no retrigger voice they hold still.
 * rn1  random per hit: each hit moves smoothly (an S-curve over one cycle) from where it is to a new
 *      random value, then holds. Offset delays it as for the other one-shots.
 *
 * Runs once per LFO update (every render block), from flash.
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "lxr0.h"

#if LXR0_LFO
#include "Soundengine/DSPAudio/lfo.h"
#include "Soundengine/DSPAudio/lxr0_lfo.h"
#include "Soundengine/DSPAudio/random.h"
#include "Soundengine/DSPAudio/wavetable.h"
#include <math.h>

enum { SHOT_HOLD, SHOT_DELAY, SHOT_RUN };

static float rnd(void) { return GetRngValue()/(float)0xffffffff; }
static float clip(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

static float spline(const float* p, float t)
{
	return 0.5f*(2.f*p[1] + (p[2] - p[0])*t + (2.f*p[0] - 5.f*p[1] + 4.f*p[2] - p[3])*t*t
			+ (3.f*p[1] - p[0] - 3.f*p[2] + p[3])*t*t*t);
}

static float splineSlope(const float* p, float t)
{
	return 0.5f*((p[2] - p[0]) + 2.f*(2.f*p[0] - 5.f*p[1] + 4.f*p[2] - p[3])*t
			+ 3.f*(3.f*p[1] - p[0] - 3.f*p[2] + p[3])*t*t);
}

// a looping shape at phase p, as lfo_calc works it out
static float shapeAt(uint8_t shape, uint32_t p)
{
	const float x = p/(float)0xffffffff;
	switch(shape)
	{
	case LFO_SINE:		return ((sine_table[p>>20]/32767.f) + 1)/2.f;
	case LFO_TRI:		return 1.f - fabsf(x*2 - 1);
	case LFO_SAW_UP:	return x;
	case LFO_SAW_DOWN:	return (0xffffffff - p)/(float)0xffffffff;
	case LFO_REC:		return p > 0x7fffffff ? 0.f : 1.f;	// one-shot square: on, then off
	case LFO_EXP_UP:	return x*x*x;
	case LFO_EXP_DOWN:	{ const float y = (0xffffffff - p)/(float)0xffffffff; return y*y*y; }
	case LFO_XTR:		{ const float y = x < 0.5f ? 2.f*x : 2.f - 2.f*x; return y*y*y; }
	default:			return 0.f;
	}
}

// the looping shape each one-shot runs
static uint8_t shotShape(uint8_t waveform)
{
	static const uint8_t shapes[] = { LFO_SINE, LFO_TRI, LFO_SAW_UP, LFO_SAW_DOWN, LFO_REC, LFO_EXP_UP, LFO_EXP_DOWN, LFO_XTR };
	return shapes[waveform - LFO_SIN1];
}

static uint8_t isShot(uint8_t waveform) { return waveform >= LFO_SIN1 && waveform <= LFO_RN1; }

float lxr0Lfo_calc(Lfo* lfo, uint32_t inc, uint8_t overflow)
{
	if(lfo->waveform == LFO_NSE)
	{
		if(!lfo->nsePrimed)
		{
			for(uint8_t i=0; i<4; i++)
				lfo->nse[i] = rnd();
			lfo->nsePrimed = 1;
		}
		if(overflow)
		{
			lfo->nse[0] = lfo->nse[1];
			lfo->nse[1] = lfo->nse[2];
			lfo->nse[2] = lfo->nse[3];
			lfo->nse[3] = rnd();
		}
		const float t = lfo->phase/(float)0xffffffff;
		lfo->nseSlope = splineSlope(lfo->nse, t);	// for a retrigger, which resets the phase
		return clip(spline(lfo->nse, t));
	}
	if(lfo->waveform == LFO_XTR)
		return shapeAt(LFO_XTR, lfo->phase);
	if(!isShot(lfo->waveform))
		return 0.f;

	if(lfo->shot == SHOT_DELAY)
	{
		if(lfo->shotDelay > inc)
		{
			lfo->shotDelay -= inc;
			return lfo->shotHeld;
		}
		lfo->shot = SHOT_RUN;
		lfo->shotPhase = 0;
	}
	if(lfo->shot == SHOT_RUN)
	{
		const uint32_t was = lfo->shotPhase;
		lfo->shotPhase += inc;
		const uint8_t done = lfo->shotPhase < was;
		const uint32_t p = done ? 0xffffffff : lfo->shotPhase;
		if(lfo->waveform == LFO_RN1)
		{
			const float x = p/(float)0xffffffff;
			lfo->shotHeld = lfo->rnFrom + (lfo->rnTo - lfo->rnFrom)*x*x*(3.f - 2.f*x);
		}
		else
			lfo->shotHeld = shapeAt(shotShape(lfo->waveform), p);
		if(done)
			lfo->shot = SHOT_HOLD;
	}
	return lfo->shotHeld;
}

void lxr0Lfo_retrigger(Lfo* lfo)
{
	if(lfo->waveform == LFO_NSE && lfo->nsePrimed)
	{
		// a new stretch from the value and slope it had at the last update
		const float d = lfo->nseSlope;
		lfo->nse[1] = lfo->value;
		lfo->nse[2] = rnd();
		lfo->nse[3] = rnd();
		lfo->nse[0] = lfo->nse[2] - 2.f*d;
		lfo->phase = 0;
		return;
	}
	if(!isShot(lfo->waveform))
		return;
	if(lfo->waveform == LFO_RN1)
	{
		lfo->rnFrom = lfo->value;
		lfo->rnTo = rnd();
	}
	lfo->shotHeld = lfo->value;
	lfo->shotPhase = 0;
	lfo->shotDelay = lfo->phaseOffset;
	lfo->shot = lfo->phaseOffset ? SHOT_DELAY : SHOT_RUN;
}
#endif
