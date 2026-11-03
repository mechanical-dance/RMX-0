/*
 * lxr0_lfo.h - lxr-0: LFO shapes after the vendor's eight (smooth random, exponential triangle, one-shots).
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef LXR0_LFO_H_
#define LXR0_LFO_H_

#include <stdint.h>

struct LfoStruct;
// the shapes from LFO_NSE on, once per LFO update. inc is this update's phase step, overflow is 1 when the phase wrapped
float lxr0Lfo_calc(struct LfoStruct* lfo, uint32_t inc, uint8_t overflow);
// the LFO's retrigger voice was hit (after the vendor code has reset the phase)
void lxr0Lfo_retrigger(struct LfoStruct* lfo);

#endif /* LXR0_LFO_H_ */
