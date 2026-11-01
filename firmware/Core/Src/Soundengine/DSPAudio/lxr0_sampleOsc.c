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
#include <math.h>
#include <string.h>
#include "Soundengine/MIDI/MidiParser.h"
#include "lxr0_samples.h"
#include "Parameters.h"
#include "menu/menu.h"

#define FRAC_BITS 17
#define SAMPLE_ROOT_HZ 311.126984f // the oscillator on coarse 0, fine 0: a sample plays at its original speed
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

void osc_setSampleTrim(OscInfo* osc, uint8_t voice)
{
	osc->trim = &activeParameters[PAR_LXR0_TRIM_START1 + voice];
}

// the part of the sample to play: the phase counts from *data. Returns 0 if the trims leave nothing.
static __inline uint32_t trimSample(const OscInfo* osc, const Lxr0Sample* sample, const int16_t** data)
{
	const uint32_t frames = sample->frames;
	*data = sample->data;
	if(!osc->trim)
		return frames;
	const uint32_t start = (uint64_t)frames*osc->trim[0]/255u;
	const uint32_t end = frames - (uint64_t)frames*osc->trim[PAR_LXR0_TRIM_END1 - PAR_LXR0_TRIM_START1]/255u;
	*data += start;
	return end > start ? end - start : 0;
}

IN_ITCM_RAM void calcUserSampleOscFmBlock(OscInfo* osc, int16_t* modBuffer, int16_t* buf, uint8_t size, const float gain)
{
	const Lxr0Sample* sample = lxr0Samples_get(osc->waveform);
	if(!sample)
	{
		memset(buf, 0, size*sizeof(int16_t));
		return;
	}
	const int16_t* data;
	const uint32_t frames = trimSample(osc, sample, &data);
	const uint32_t looped = sample->looped;
	if(!frames)
	{
		memset(buf, 0, size*sizeof(int16_t));
		return;
	}

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
	const int16_t* data;
	const uint32_t frames = trimSample(osc, sample, &data);
	const uint32_t looped = sample->looped;
	if(!frames)
	{
		memset(buf, 0, size*sizeof(int16_t));
		return;
	}

	for(uint8_t i=0; i<size; i++)
	{
		const int16_t out = readSample(data, frames, looped, getPosition(osc));
		advance(osc, frames, looped);
		osc->output = out;
		buf[i] = out * gain;
	}
}

// coarse 0, fine 0 is a file's original speed at any sample rate; each coarse step is a semitone
void osc_calcUserSampleFreq(OscInfo* osc)
{
	const Lxr0Sample* sample = lxr0Samples_get(osc->waveform);
	const float rate = sample ? (float)sample->rate : 44100.f;
	float currentFreq;
	if(osc->cvFreq == -1.0) // no CV, as osc_calcWavetableFreq
	{
		currentFreq = osc->freq*osc->pitchMod*osc->modNodeValue;
	}
	else
	{
		const float cent = midiParser_calcDetune(osc->midiFreq&0xff);
		const float detune = powf(2.f, ((osc->midiFreq>>8) - 60)/12.f);
		currentFreq = detune*osc->cvFreq*cent*osc->pitchMod*osc->modNodeValue;
	}
	osc->phaseInc = (uint32_t)((currentFreq/SAMPLE_ROOT_HZ) * (rate/44100.f) * 131072.f);
}
#endif
