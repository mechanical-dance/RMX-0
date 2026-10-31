/*
 * lxr0_sampleOsc.c - lxr-0: sample playback for waveforms above the built-in ones.
 *
 * Follows calcUserSampleOscBlock of the original LXR and LXR02Open (17 fractional bits, linear
 * interpolation, one shots stop at the end, FM moves the read position). osc.sampleHi extends the
 * 32-bit phase, which on its own runs out after 32768 frames.
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "Soundengine/DSPAudio/Oscillator.h"

#if LXR0_SAMPLES
#include <string.h>
#include "lxr0_samples.h"

#define FRAC_BITS 17
#define FRAC_MASK ((1u<<FRAC_BITS)-1u)
#define ONE_OVER_FRAC 0.00000762939453125f // 1 / 2^17

static __inline uint64_t getPosition(const OscInfo* osc)
{
	return ((uint64_t)osc->sampleHi << 32) | osc->phase;
}

static __inline int16_t readSample(const int16_t* data, const uint32_t frames, const uint32_t looped, const uint64_t position)
{
	uint32_t itg = (uint32_t)(position >> FRAC_BITS);
	if(itg >= frames)
	{
		if(!looped)
			return 0;
		itg %= frames;
	}
	// a one shot's last frame fades to the zeros stored after it
	const uint32_t next = (looped && itg + 1u >= frames) ? 0 : itg + 1u;
	const float frac = ((uint32_t)position & FRAC_MASK) * ONE_OVER_FRAC;
	return (int16_t)(data[itg] + frac*(data[next] - data[itg]));
}

static __inline void advance(OscInfo* osc, const uint32_t frames, const uint32_t looped)
{
	const uint64_t end = (uint64_t)frames << FRAC_BITS;
	uint64_t position = getPosition(osc) + osc->phaseInc;
	if(position >= end)
		position = looped ? position % end : end; // a one shot waits at the end
	osc->phase = (uint32_t)position;
	osc->sampleHi = (uint32_t)(position >> 32);
}

IN_ITCM_RAM void calcUserSampleOscFmBlock(OscInfo* osc, int16_t* modBuffer, int16_t* buf, uint8_t size, const float gain)
{
	const Lxr0Sample* sample = lxr0Samples_get(osc->waveform);
	if(!sample)
	{
		memset(buf, 0, size*sizeof(int16_t));
		return;
	}
	const int16_t* data = sample->data;
	const uint32_t frames = sample->frames;
	const uint32_t looped = sample->looped;

	for(uint8_t i=0; i<size; i++)
	{
		const uint32_t mod = ((uint32_t)(modBuffer[i]*osc->fmMod)) << 14;
		const int16_t out = readSample(data, frames, looped, getPosition(osc) + mod);
		advance(osc, frames, looped);
		osc->output = out;
		buf[i] = out * gain;
	}
}

IN_ITCM_RAM void calcUserSampleOscBlock(OscInfo* osc, int16_t* buf, const uint8_t size, const float gain)
{
	const Lxr0Sample* sample = lxr0Samples_get(osc->waveform);
	if(!sample)
	{
		memset(buf, 0, size*sizeof(int16_t));
		return;
	}
	const int16_t* data = sample->data;
	const uint32_t frames = sample->frames;
	const uint32_t looped = sample->looped;

	for(uint8_t i=0; i<size; i++)
	{
		const int16_t out = readSample(data, frames, looped, getPosition(osc));
		advance(osc, frames, looped);
		osc->output = out;
		buf[i] = out * gain;
	}
}

// the increment that plays at the file's own rate and pitch when the oscillator is on F1 (43.65 Hz)
void osc_calcUserSampleFreq(OscInfo* osc)
{
	const Lxr0Sample* sample = lxr0Samples_get(osc->waveform);
	const float rate = sample ? (float)sample->rate : REAL_FS;
	const float currentFreq = osc->freq*osc->cvFreq*osc->pitchMod*osc->modNodeValue;
	osc->phaseInc = (uint32_t)((currentFreq/43.6535289291f) * (rate/REAL_FS) * 131072.f);
}
#endif
