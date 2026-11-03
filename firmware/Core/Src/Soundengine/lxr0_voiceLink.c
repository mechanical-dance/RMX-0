/*
 * lxr0_voiceLink.c - lxr-0: drums 1-3 linked as one instrument.
 *
 * Unison and chord: the lowest linked drum's trigger and pitch CV play every linked drum. Unison
 * spreads them by det (pitch) and wid (pan), chord takes its intervals from each drum's tuning and
 * spreads them by wid. Both scale each drum by 1/sqrt(drums), so linking doesn't jump in level.
 * Poly: each trigger goes to one drum, which keeps the pitch it was given. A free drum (its amp
 * envelope has ended) is used first, the least recently used one; if none is free the oldest or the
 * highest note is replaced, through the retrigger declick. The other linked drums' triggers and
 * pitch CV are ignored while linked. All of this runs at trigger and block rate, from flash.
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "lxr0.h"

#if LXR0_LINK
#include <math.h>
#include <string.h>
#include "main.h"
#include "Parameters.h"
#include "menu/menu.h"
#include "Soundengine/lxr0_voiceLink.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/MIDI/MidiVoiceControl.h"

#define DRUMS 3
#define DETUNE_MAX_CENTS 50.f	// det 127: the outer drums sit this far either side
#define FOLLOW_BLOCKS 3			// poly: the newest note takes CV for ~4 ms (blocks), as the CV can trail its trigger
enum { MODE_OFF, MODE_UNISON, MODE_CHORD, MODE_POLY };

static uint8_t settings[PAR_LXR0_LINK_ALLOC - PAR_LXR0_LINK_VOICES + 1];
static uint8_t linked;		// drums in the link now, 0 if it's off
static uint8_t mode;
static uint8_t lead;
static float leadCv = -1.f;
static uint32_t noteCount;
static uint32_t age[DRUMS];
static float pitchKey[DRUMS];
static uint8_t follow[DRUMS];

static uint8_t countBits(uint8_t m) { return (m&1) + ((m>>1)&1) + ((m>>2)&1); }

static uint8_t solo;		// a double-tapped drum is edited on its own; group edits are off

uint8_t lxr0Link_voices(void) { return linked; }
uint8_t lxr0Link_solo(void) { return solo; }
void lxr0Link_setSolo(uint8_t on) { solo = on; }
uint8_t lxr0Link_lead(void) { return lead; }
uint8_t lxr0Link_route(uint8_t voice) { return (voice < DRUMS && (linked & (1<<voice))) ? lead : voice; }

// detune, pan spread and level for each drum
static void apply(void)
{
	const uint8_t n = countBits(linked);
	uint8_t rank = 0;
	for(uint8_t v=0; v<DRUMS; v++)
	{
		DrumVoice* d = &voiceArray[v];
		float detune = 1.f, gain = 1.f;
		int8_t pan = 0;
		if(linked & (1<<v))
		{
			const float pos = 2.f*rank/(n - 1) - 1.f; // -1 to 1 across the linked drums
			rank++;
			if(mode == MODE_UNISON)
				detune = powf(2.f, pos*settings[PAR_LXR0_LINK_DETUNE - PAR_LXR0_LINK_VOICES]/127.f*DETUNE_MAX_CENTS/1200.f);
			if(mode == MODE_UNISON || mode == MODE_CHORD)
			{
				pan = (int8_t)lrintf(pos*settings[PAR_LXR0_LINK_WIDTH - PAR_LXR0_LINK_VOICES]/127.f*63.f);
				gain = 1.f/sqrtf((float)n);
			}
		}
		d->linkDetune = detune;
		d->modOsc.pitchMod = detune;
		d->linkPan = pan;
		d->linkGain = gain;
	}
}

void lxr0Link_poll(void)
{
	for(uint8_t v=0; v<DRUMS; v++)
		if(follow[v])
			follow[v]--;
	if(!memcmp(settings, &activeParameters[PAR_LXR0_LINK_VOICES], sizeof settings))
		return;
	memcpy(settings, &activeParameters[PAR_LXR0_LINK_VOICES], sizeof settings);
	const uint8_t was = linked, wasPoly = linked && mode == MODE_POLY, wasLead = lead;
	const uint8_t m = settings[0] & 7;
	mode = settings[PAR_LXR0_LINK_MODE - PAR_LXR0_LINK_VOICES];
	linked = (mode != MODE_OFF && mode <= MODE_POLY && countBits(m) >= 2) ? m : 0;
	lead = linked ? (uint8_t)__builtin_ctz(linked) : 0;
	// in poly the lead drum holds a note's pitch, not the CV: give it the CV back
	if(wasPoly && (!linked || mode != MODE_POLY || lead != wasLead))
		Drum_setCv1VOct(wasLead, leadCv);
	if(linked != was || lead != wasLead)
	{
		// the CV only reaches the link when it changes: start from the CV the lead has now
		leadCv = voiceArray[lead].osc.cvFreq;
		// the drums that just joined start from the lead's pitch CV
		for(uint8_t v=0; v<DRUMS; v++)
			if((linked & (1<<v)) && !(was & (1<<v)) && v != lead)
				Drum_setCv1VOct(v, voiceArray[lead].osc.cvFreq);
		memset(follow, 0, sizeof follow);
	}
	apply();
}

static uint8_t isFree(uint8_t v)
{
	const SlopeEg2* eg = &voiceArray[v].oscVolEg;
	return eg->state == EG_STOPPED || (eg->state == EG_D && eg->value <= 0);
}

static uint8_t allocate(void)
{
	uint8_t best = 0xff;
	for(uint8_t v=0; v<DRUMS; v++)
		if((linked & (1<<v)) && isFree(v) && (best == 0xff || age[v] < age[best]))
			best = v;
	if(best != 0xff)
		return best;
	const uint8_t highest = settings[PAR_LXR0_LINK_ALLOC - PAR_LXR0_LINK_VOICES];
	for(uint8_t v=0; v<DRUMS; v++)
	{
		if(!(linked & (1<<v)))
			continue;
		if(best == 0xff || (highest ? pitchKey[v] > pitchKey[best] : age[v] < age[best]))
			best = v;
	}
	return best;
}

uint8_t lxr0Link_noteOn(uint8_t voice, uint8_t note, uint8_t vel)
{
	if(voice >= DRUMS || !(linked & (1<<voice)))
		return 0;
	if(voice != lead)
		return 1; // the lead's trigger plays the link
	if(mode != MODE_POLY)
	{
		for(uint8_t v=0; v<DRUMS; v++)
			if(linked & (1<<v))
				voiceControl_queueTrigger(v, note, vel);
		return 1;
	}
	// triggers come from SysTick and from the main loop (MIDI, buttons)
	const uint32_t primask = __get_PRIMASK();
	__disable_irq();
	const uint8_t v = allocate();
	age[v] = ++noteCount;
	pitchKey[v] = (leadCv > 0.f ? leadCv : 1.f)*powf(2.f, (note - 60)/12.f);
	memset(follow, 0, sizeof follow);
	follow[v] = FOLLOW_BLOCKS;
	Drum_setCv1VOct(v, leadCv);
	voiceControl_queueTrigger(v, note, vel);
	if(!primask)
		__enable_irq();
	return 1;
}

uint8_t lxr0Link_cv(uint8_t voice, float freq)
{
	if(voice >= DRUMS || !(linked & (1<<voice)))
		return 0;
	if(voice != lead)
		return 1;
	if(mode != MODE_POLY)
	{
		for(uint8_t v=0; v<DRUMS; v++)
			if(linked & (1<<v))
				Drum_setCv1VOct(v, freq);
		return 1;
	}
	// poly: a drum holds its pitch, except the newest for a few blocks after its trigger
	leadCv = freq;
	for(uint8_t v=0; v<DRUMS; v++)
		if(follow[v])
			Drum_setCv1VOct(v, freq);
	return 1;
}
#endif
