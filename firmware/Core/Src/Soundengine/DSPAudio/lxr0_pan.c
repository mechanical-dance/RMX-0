/*
 * lxr0_pan.c - lxr-0: pan follows its value while a note sounds, on all six voices.
 *
 * The vendor works out a voice's left and right gains (triggeredPanL/R) only when it's triggered, so the pan
 * knob, a pan LFO, a velocity route or the link's width moved only the next hit. Here each voice's gains are
 * worked out again in any block where its pan has changed, with the triggers' own formula. The gains step once
 * per block, with no ramp, as the LFOs update at block rate. Runs from flash at block rate.
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "lxr0.h"

#if LXR0_PAN
#include <math.h>
#include "Soundengine/DSPAudio/lxr0_pan.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/CymbalVoice.h"
#include "Soundengine/DSPAudio/HiHat.h"

static int16_t last[6] = {-1, -1, -1, -1, -1, -1};

static void gains(uint8_t i, int16_t pan, float* l, float* r)
{
	if(pan == last[i])
		return;
	last[i] = pan;
	*l = sqrtf((127 - pan)/127.f);
	*r = sqrtf(pan/127.f);
}

void lxr0Pan_update(void)
{
	for(uint8_t v=0; v<3; v++)
	{
		DrumVoice* d = &voiceArray[v];
#if LXR0_LINK
		int16_t pan = d->pan + d->linkPan; // as Drum_updatePan: a link spreads its voices around their own pan
		pan = pan < 0 ? 0 : pan > 127 ? 127 : pan;
#else
		const int16_t pan = d->pan;
#endif
		gains(v, pan, &d->triggeredPanL, &d->triggeredPanR);
	}
	gains(3, snareVoice.pan, &snareVoice.triggeredPanL, &snareVoice.triggeredPanR);
	gains(4, cymbalVoice.pan, &cymbalVoice.triggeredPanL, &cymbalVoice.triggeredPanR);
	gains(5, hatVoice.pan, &hatVoice.triggeredPanL, &hatVoice.triggeredPanR);
}
#endif
