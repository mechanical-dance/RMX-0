/*
 * lxr0_declick.c - lxr-0: removes the step a retrigger adds to a voice that is still sounding.
 *
 * A trigger resets the oscillator's phase while the old note is at some level, so the output
 * jumps whatever the envelope does. The last output sample is added to the new note and fades to
 * zero, and the new note itself starts exactly as it would from silence. No delay is added.
 * This runs before the voice's decimator, which still holds the old level for its first samples.
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "lxr0.h"

#if LXR0_DECLICK
#include "Soundengine/config.h"
#include "Soundengine/DSPAudio/lxr0_declick.h"

#define FADE_SAMPLES 132 // 3 ms at 44.1 kHz
#define ONE_OVER_STEPS (1.f/(FADE_SAMPLES + 1))
#define VOICES 6 // hats share one voice

typedef struct
{
	float level;
	int16_t last;
	uint8_t remaining;
} Declick;

INDTCMZ static Declick declick[VOICES];

uint8_t lxr0Declick_arm(const uint8_t voice)
{
	Declick* d = &declick[voice];
	if(!d->last)
		return 0;
	d->level = d->last; // during a fade this includes what's left of the older note
	d->remaining = FADE_SAMPLES;
	return 1;
}

IN_ITCM_RAM void lxr0Declick_process(const uint8_t voice, int16_t* buf, const uint8_t size)
{
	Declick* d = &declick[voice];
	for(uint8_t i=0; i<size && d->remaining; i++, d->remaining--)
	{
		// smoothstep from 1 to 0: flat at both ends, so the fade adds no corner of its own
		const float x = (FADE_SAMPLES + 1 - d->remaining)*ONE_OVER_STEPS;
		const int32_t out = buf[i] + (int32_t)(d->level*(1.f - x*x*(3.f - 2.f*x)));
		buf[i] = out > 32767 ? 32767 : out < -32768 ? -32768 : (int16_t)out;
	}
	d->last = buf[size-1];
}
#endif
