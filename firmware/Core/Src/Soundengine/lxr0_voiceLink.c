/*
 * lxr0_voiceLink.c - lxr-0: drums 1-3 linked as one instrument.
 *
 * Unison and chord: the lowest linked drum's trigger and pitch CV play every linked drum. Unison
 * spreads them by det (pitch) and wid (pan), chord takes its intervals from each drum's tuning and
 * spreads them by wid. Each drum keeps its own volume (0.5.8; before, 1/sqrt(drums)), so a group is louder.
 * Poly: each trigger goes to one drum, which keeps the pitch it was given. A free drum (its amp
 * envelope has ended) is used first, the least recently used one; if none is free the oldest or the
 * highest note is replaced, through the retrigger declick. The other linked drums' triggers and
 * pitch CV are ignored while linked. All of this runs at trigger and block rate, from flash.
 * 0.5.7: with MIDI note-offs, poly takes a drum whose key is released before one still held.
 *
 * Glide (0.5.4, types 0.5.6, per drum 0.5.7): each drum glides with its own settings; a unison or chord
 * group slides as one, with its lead's settings; poly doesn't glide. It always glides, through silence and repeated hits,
 * as an analog portamento does. Note, pitch CV and tuning changes all glide, as it follows the
 * oscillator's base pitch. It rides on the link detune multiplier. The knob sets a time T, 25 ms to 8 s:
 *   lcr  linear constant rate: T per octave, even in pitch, so bigger jumps take longer
 *   lct  linear constant time: every slide takes T from where it is, whatever the interval
 *   exp  exponential (RC lag): fast, then easing in; T to within 1% of the interval
 *
 * Legato (0.5.7): drums 1-3 keep the MIDI keys held on them. With lgt on, a note that comes while a key
 * is held and the drum still sounds changes only its pitch: no envelope, transient, LFO or filter restart.
 * Letting go of the newest key goes back to the newest one still held, the same way. A group plays legato
 * with its lead's setting; poly doesn't. Triggers and CV always play a new note.
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
#include "Soundengine/MIDI/MidiParser.h"
#include "Soundengine/config.h"
#include "Soundengine/DSPAudio/Oscillator.h"

#define DRUMS 3
#define DETUNE_MAX_CENTS 50.f	// det 127: the outer drums sit this far either side
#define GLIDE_FAST_S 0.025f		// glide 1: T in seconds
#define GLIDE_SLOW_S 8.f		// glide 127
#define GLIDE_SNAP 0.0005f		// exp: closer than this (octaves, 0.6 cents) is there
enum { GLIDE_LCR, GLIDE_LCT, GLIDE_EXP };
#define FOLLOW_BLOCKS 3			// poly: the newest note takes CV for ~4 ms (blocks), as the CV can trail its trigger
enum { MODE_OFF, MODE_UNISON, MODE_CHORD, MODE_POLY };

#define HELD 8				// keys held per drum; more drop the oldest
#define SET(p) settings[(p) - PAR_LXR0_LINK_VOICES]

static uint8_t settings[PAR_LXR0_LINK_LEGATO3 - PAR_LXR0_LINK_VOICES + 1];
static const uint16_t glidePar[DRUMS] = {PAR_LXR0_LINK_GLIDE, PAR_LXR0_LINK_GLIDE2, PAR_LXR0_LINK_GLIDE3};
static const uint16_t typePar[DRUMS] = {PAR_LXR0_LINK_GLIDE_TYPE, PAR_LXR0_LINK_GLIDE_TYPE2, PAR_LXR0_LINK_GLIDE_TYPE3};
static uint8_t linked;		// drums in the link now, 0 if it's off
static uint8_t mode;
static uint8_t lead;
static float leadCv = -1.f;
static uint32_t noteCount;
static uint32_t age[DRUMS];
static float pitchKey[DRUMS];
static uint8_t follow[DRUMS];
static float detuneOf[DRUMS] = {1.f, 1.f, 1.f};	// each drum's link detune, which glide multiplies
// a glide, one per drum: the drum on its own, or the group it leads
typedef struct
{
	uint8_t on, type;
	uint8_t voices;	// the drums it moved last block
	float step;		// lcr: octaves per block
	float blocks;	// lct: blocks a slide takes
	float rate;		// lct: this slide's octaves per block
	float decay;	// exp: what's left of the distance after a block
	float offset;	// octaves the drums sit from their pitch now
	float pitch;	// their base pitch last block, in octaves; 0 before the first block
} Glide;
static Glide glide[DRUMS];

// MIDI keys held on a drum (MIDI only: the main loop), oldest first
typedef struct
{
	uint8_t n;
	uint8_t chan[HELD], key[HELD], note[HELD];
} Keys;
static Keys keys[DRUMS];
static volatile uint8_t clearKeys;		// the link changed: the keys held mean nothing now
static volatile uint8_t legatoNote[DRUMS], legatoDue[DRUMS];	// a pitch to take at the next block
static uint8_t polyChan[DRUMS], polyKey[DRUMS];
static uint8_t polyHeld;				// poly drums whose MIDI key is still down

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
				pan = (int8_t)lrintf(pos*settings[PAR_LXR0_LINK_WIDTH - PAR_LXR0_LINK_VOICES]/127.f*63.f);
			// 0.5.8: no level scaling: each linked drum plays at its own volume (0.5.0-0.5.7 scaled by 1/sqrt(drums))
		}
		detuneOf[v] = detune;
		d->linkDetune = detune;
		d->modOsc.pitchMod = detune;
		d->linkPan = pan;
		d->linkGain = gain;
	}
	for(uint8_t v=0; v<DRUMS; v++)
	{
		Glide* gl = &glide[v];
		const uint8_t g = SET(glidePar[v]);
		const float t = GLIDE_FAST_S*powf(GLIDE_SLOW_S/GLIDE_FAST_S, (g > 0 ? g - 1 : 0)/126.f), dt = OUTPUT_DMA_SIZE/44100.f;
		gl->on = g > 0;
		gl->type = SET(typePar[v]);
		if(gl->type > GLIDE_EXP)
			gl->type = GLIDE_LCR;
		gl->step = dt/t;
		gl->blocks = t/dt;
		gl->decay = expf(-dt*4.6052f/t); // 1% left after t
	}
}

// the drums a drum's notes play, legato or glide: itself, the group it leads in unison and chord, or none
static uint8_t monoVoices(uint8_t v)
{
	if(linked & (1<<v))
		return (v == lead && mode != MODE_POLY) ? linked : 0;
	return (uint8_t)(1<<v);
}

void lxr0Link_poll(void)
{
	for(uint8_t v=0; v<DRUMS; v++)
	{
		if(follow[v])
			follow[v]--;
		if(legatoDue[v])
		{
			// before the trigger queue, so a new note in this block still plays as a note
			legatoDue[v] = 0;
			const uint8_t voices = monoVoices(v);
			for(uint8_t d=0; d<DRUMS; d++)
				if(voices & (1<<d))
				{
					osc_setBaseNote(&voiceArray[d].osc, legatoNote[v]);
					osc_setBaseNote(&voiceArray[d].modOsc, legatoNote[v]);
				}
		}
	}
	if(!memcmp(settings, &activeParameters[PAR_LXR0_LINK_VOICES], sizeof settings))
		return;
	memcpy(settings, &activeParameters[PAR_LXR0_LINK_VOICES], sizeof settings);
	const uint8_t was = linked, wasPoly = linked && mode == MODE_POLY, wasLead = lead, wasMode = mode;
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
	if(linked != was || lead != wasLead || (linked && mode != wasMode))
	{
		polyHeld = 0;
		clearKeys = 1;
	}
	apply();
}

static uint8_t isFree(uint8_t v)
{
	const SlopeEg2* eg = &voiceArray[v].oscVolEg;
	return eg->state == EG_STOPPED || (eg->state == EG_D && eg->value <= 0);
}

// the note to replace among these drums: the oldest, or the highest
static uint8_t replace(uint8_t among)
{
	const uint8_t highest = SET(PAR_LXR0_LINK_ALLOC);
	uint8_t best = 0xff;
	for(uint8_t v=0; v<DRUMS; v++)
	{
		if(!(among & (1<<v)))
			continue;
		if(best == 0xff || (highest ? pitchKey[v] > pitchKey[best] : age[v] < age[best]))
			best = v;
	}
	return best;
}

// a free drum first, the least recently used; then one whose key is up; then one still held
static uint8_t allocate(void)
{
	uint8_t best = 0xff;
	for(uint8_t v=0; v<DRUMS; v++)
		if((linked & (1<<v)) && isFree(v) && (best == 0xff || age[v] < age[best]))
			best = v;
	if(best != 0xff)
		return best;
	best = replace(linked & ~polyHeld);
	return best != 0xff ? best : replace(linked);
}

// a poly note. midi: its key is held until its note-off; other notes count as let go at once
static void polyNote(uint8_t note, uint8_t vel, uint8_t midi, uint8_t chan, uint8_t key)
{
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
	if(midi)
	{
		polyChan[v] = chan;
		polyKey[v] = key;
		polyHeld |= (uint8_t)(1<<v);
	}
	else
		polyHeld &= (uint8_t)~(1<<v);
	if(!primask)
		__enable_irq();
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
	polyNote(note, vel, 0, 0, 0);
	return 1;
}

static void takeClear(void)
{
	if(clearKeys)
	{
		clearKeys = 0;
		memset(keys, 0, sizeof keys);
	}
}

static uint8_t legatoOn(uint8_t v) { return activeParameters[PAR_LXR0_LINK_LEGATO1 + v] != 0; }

static void legato(uint8_t v, uint8_t note)
{
	legatoNote[v] = note;
	legatoDue[v] = 1;
}

// take a key off a drum's list. Returns 1 if it was the newest
static uint8_t keyUp(Keys* k, uint8_t chan, uint8_t key)
{
	for(uint8_t i=0; i<k->n; i++)
		if(k->chan[i] == chan && k->key[i] == key)
		{
			const uint8_t newest = i == k->n - 1;
			k->n--;
			for(; i<k->n; i++)
			{
				k->chan[i] = k->chan[i+1];
				k->key[i] = k->key[i+1];
				k->note[i] = k->note[i+1];
			}
			return newest;
		}
	return 0;
}

uint8_t lxr0Link_midiNoteOn(uint8_t voice, uint8_t chan, uint8_t key, uint8_t note, uint8_t vel)
{
	if(voice >= DRUMS)
		return 0;
	takeClear();
	if(linked & (1<<voice))
	{
		if(voice != lead)
			return 1; // the lead's notes play the link
		if(mode == MODE_POLY)
		{
			polyNote(note, vel, 1, chan, key);
			return 1;
		}
	}
	Keys* k = &keys[voice];
	const uint8_t held = k->n;
	keyUp(k, chan, key);
	if(k->n == HELD)
		keyUp(k, k->chan[0], k->key[0]);
	k->chan[k->n] = chan;
	k->key[k->n] = key;
	k->note[k->n] = note;
	k->n++;
	if(held && legatoOn(voice) && !isFree(voice))
	{
		legato(voice, note);
		return 1;
	}
	return 0;
}

void lxr0Link_midiNoteOff(uint8_t chan, uint8_t key)
{
	takeClear();
	for(uint8_t v=0; v<DRUMS; v++)
	{
		Keys* k = &keys[v];
		if(keyUp(k, chan, key) && k->n && monoVoices(v) && legatoOn(v) && !isFree(v))
			legato(v, k->note[k->n - 1]); // back to the newest key still held
	}
	const uint32_t primask = __get_PRIMASK();
	__disable_irq();
	for(uint8_t v=0; v<DRUMS; v++)
		if((polyHeld & (1<<v)) && polyChan[v] == chan && polyKey[v] == key)
			polyHeld &= (uint8_t)~(1<<v);
	if(!primask)
		__enable_irq();
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

// the oscillator's pitch before its modulation, as Oscillator.c and lxr0_sampleOsc.c work it out
static float basePitch(const OscInfo* o)
{
	if(o->cvFreq == -1.f)
		return o->freq;
	return powf(2.f, ((o->midiFreq>>8) - 60)/12.f)*midiParser_calcDetune(o->midiFreq&0xff)*o->cvFreq;
}

void lxr0Link_glide(void)
{
	float ratio[DRUMS] = {1.f, 1.f, 1.f};
	for(uint8_t p=0; p<DRUMS; p++)
	{
		Glide* g = &glide[p];
		const uint8_t voices = monoVoices(p);
		if(voices != g->voices) // it moves other drums now: nothing to glide from
			g->pitch = 0.f;
		g->voices = voices;
		if(!voices)
		{
			g->offset = 0.f;
			continue;
		}
		const float f = basePitch(&voiceArray[p].osc);
		const float pitch = f > 0.f ? log2f(f) : 0.f;
		if(!g->on)
			g->offset = 0.f;
		else if(g->pitch != 0.f && pitch != g->pitch)
		{
			g->offset += g->pitch - pitch; // from wherever it is now
			g->rate = fabsf(g->offset)/g->blocks;
		}
		g->pitch = pitch;
		if(g->type == GLIDE_EXP)
		{
			g->offset *= g->decay;
			if(fabsf(g->offset) < GLIDE_SNAP)
				g->offset = 0.f;
		}
		else
		{
			const float step = g->type == GLIDE_LCT ? g->rate : g->step;
			if(g->offset > step)
				g->offset -= step;
			else if(g->offset < -step)
				g->offset += step;
			else
				g->offset = 0.f;
		}
		if(g->offset != 0.f)
		{
			const float r = exp2f(g->offset);
			for(uint8_t v=0; v<DRUMS; v++)
				if(voices & (1<<v))
					ratio[v] = r;
		}
	}
	for(uint8_t v=0; v<DRUMS; v++)
	{
		const float m = detuneOf[v]*ratio[v];
		voiceArray[v].linkDetune = m;
		voiceArray[v].modOsc.pitchMod = m;
	}
}
#endif