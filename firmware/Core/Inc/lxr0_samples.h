/*
 * lxr0_samples.h - lxr-0: user samples as oscillator waveforms, installed from SD to internal flash.
 *
 * As on the original LXR and LXR02Open: waveforms from OSC_SAMPLE_START up play samples held in flash.
 * Added: rates other than 44.1 kHz, stereo files summed to mono, 24-bit files reduced to 16, and
 * samples longer than the 32-bit phase can index (about 0.74 s at the reference pitch).
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef LXR0_SAMPLES_H_
#define LXR0_SAMPLES_H_

#include <stdint.h>
#include "lxr0.h"

#if LXR0_SAMPLES
#define LXR0_SAMPLE_WAVE_START 6	// == NUM_OSC_WAVEFORMS, the first waveform that is a sample
#define LXR0_SAMPLE_MAX 120			// waveform numbers must stay below 128

typedef struct
{
	const int16_t*	data;	// mono, followed by zeros
	uint32_t		frames;
	uint32_t		rate;	// Hz
	uint32_t		looped;
	char			name[8];// file name stem, space padded
} Lxr0Sample;

uint8_t lxr0Samples_init(void);	// validates the flash table, returns the number of samples
const Lxr0Sample* lxr0Samples_get(uint8_t waveform);	// 0 if this waveform has no sample
void lxr0Samples_installFromSd(void);	// installs /samples then /loops and resets; if it can't start, says why and returns with flash untouched
#endif

#endif /* LXR0_SAMPLES_H_ */
