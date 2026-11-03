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
// trims only: an untrimmed sample plays exactly as stored
#define START_FADE_SAMPLES 44 // 1 ms at 44.1 kHz, whatever the pitch
#define END_FADE_MS 3u
#define LOOP_XFADE_MS 5u

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

// the part of the sample to play. An untrimmed sample takes the plain path, unchanged.
typedef struct
{
	const int16_t* data; // the phase counts from here
	uint32_t frames;
	uint32_t looped;
	uint32_t trimmed;
	uint32_t fadeIn;     // a start trim: fade in over the first output samples
	uint32_t fadeOut;    // a one shot's end trim: frames to fade out over
	uint32_t xfade;      // a trimmed loop: frames to crossfade over before the end
	uint32_t preroll;    // frames before the loop start to crossfade from, up to xfade
} SampleSpan;

static __inline void trimSample(const OscInfo* osc, const Lxr0Sample* sample, SampleSpan* s)
{
	const uint32_t all = sample->frames;
	uint32_t start = 0, cut = 0;
	if(osc->trim)
	{
		start = (uint64_t)all*osc->trim[0]/255u;
		cut = (uint64_t)all*osc->trim[PAR_LXR0_TRIM_END1 - PAR_LXR0_TRIM_START1]/255u;
	}
	const uint32_t end = all - cut;
	s->data = sample->data + start;
	s->frames = end > start ? end - start : 0;
	s->looped = sample->looped;
	s->trimmed = start || cut;
	s->fadeIn = start; // the start frame until setTrimFades
	s->fadeOut = cut;  // the cut frames until setTrimFades
}

// the fade lengths, for a trimmed sample only
static __inline void setTrimFades(SampleSpan* s, const uint32_t rate)
{
	const uint32_t start = s->fadeIn, cut = s->fadeOut;
	const uint32_t half = s->frames/2;
	s->fadeIn = start != 0;
	s->fadeOut = s->xfade = s->preroll = 0;
	if(s->looped)
	{
		const uint32_t x = rate*LOOP_XFADE_MS/1000u;
		s->xfade = x < half ? x : half;
		s->preroll = start < s->xfade ? start : s->xfade;
	}
	else if(cut)
	{
		const uint32_t x = rate*END_FADE_MS/1000u;
		s->fadeOut = x < half ? x : half;
	}
}

static __inline float lerpFrame(const int16_t* data, const int32_t itg, const float frac)
{
	return data[itg] + frac*(data[itg + 1] - data[itg]);
}

static __inline int16_t readTrimmed(OscInfo* osc, const SampleSpan* s, const uint64_t position)
{
	float out = readSample(s->data, s->frames, s->looped, position);
	const uint32_t itg = (uint32_t)(position >> FRAC_BITS);
	const float frac = ((uint32_t)position & FRAC_MASK) * ONE_OVER_FRAC;
	if(itg + s->xfade >= s->frames && itg < s->frames && s->xfade)
	{
		// blend into what plays after the wrap, so the wrap itself lands on the same sample
		const uint32_t k = itg + s->xfade - s->frames;
		const float t = (k + frac)/s->xfade;
		out += t*(lerpFrame(s->data, (int32_t)k - (int32_t)s->preroll, frac) - out);
	}
	else if(itg + s->fadeOut >= s->frames && itg < s->frames && s->fadeOut)
	{
		out *= (s->frames - itg - frac)/s->fadeOut;
	}
	if(s->fadeIn && osc->sampleFade < START_FADE_SAMPLES)
	{
		osc->sampleFade++;
		out *= osc->sampleFade*(1.f/(START_FADE_SAMPLES + 1));
	}
	return (int16_t)out;
}

// after a trimmed loop's crossfade, play on from where its blend ended
static __inline void advanceTrimmed(OscInfo* osc, const SampleSpan* s)
{
	const uint64_t end = (uint64_t)s->frames << FRAC_BITS;
	uint64_t position = getPosition(osc) + osc->phaseInc;
	if(position >= end)
	{
		if(s->looped)
		{
			const uint64_t restart = (uint64_t)(s->xfade - s->preroll) << FRAC_BITS;
			position = restart + (position - end) % (end - restart);
		}
		else
		{
			position = end; // a one shot waits at the end
		}
	}
	osc->phase = (uint32_t)position;
	osc->sampleHi = (uint32_t)(position >> 32);
}

// a trimmed sample, with or without FM (modBuffer NULL): one shared copy, to keep ITCM free
IN_ITCM_RAM __attribute__((noinline, optimize("Os"))) static void calcTrimmedBlock(OscInfo* osc, SampleSpan* s, const uint32_t rate, const int16_t* modBuffer, int16_t* buf, const uint8_t size, const float gain)
{
	setTrimFades(s, rate);
	for(uint8_t i=0; i<size; i++)
	{
		const uint32_t mod = modBuffer ? ((uint32_t)(modBuffer[i]*osc->fmMod)) << 14 : 0;
		const int16_t out = readTrimmed(osc, s, getPosition(osc) + mod);
		advanceTrimmed(osc, s);
		osc->output = out;
		buf[i] = out * gain;
	}
}

IN_ITCM_RAM void calcUserSampleOscFmBlock(OscInfo* osc, int16_t* modBuffer, int16_t* buf, uint8_t size, const float gain)
{
	const Lxr0Sample* sample = lxr0Samples_get(osc->waveform);
	if(!sample)
	{
		memset(buf, 0, size*sizeof(int16_t));
		return;
	}
	SampleSpan s;
	trimSample(osc, sample, &s);
	if(!s.frames)
	{
		memset(buf, 0, size*sizeof(int16_t));
		return;
	}
	if(s.trimmed)
	{
		calcTrimmedBlock(osc, &s, sample->rate, modBuffer, buf, size, gain);
		return;
	}

	for(uint8_t i=0; i<size; i++)
	{
		const uint32_t mod = ((uint32_t)(modBuffer[i]*osc->fmMod)) << 14;
		const int16_t out = readSample(s.data, s.frames, s.looped, getPosition(osc) + mod);
		advance(osc, s.frames, s.looped);
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
	SampleSpan s;
	trimSample(osc, sample, &s);
	if(!s.frames)
	{
		memset(buf, 0, size*sizeof(int16_t));
		return;
	}
	if(s.trimmed)
	{
		calcTrimmedBlock(osc, &s, sample->rate, NULL, buf, size, gain);
		return;
	}

	for(uint8_t i=0; i<size; i++)
	{
		const int16_t out = readSample(s.data, s.frames, s.looped, getPosition(osc));
		advance(osc, s.frames, s.looped);
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
