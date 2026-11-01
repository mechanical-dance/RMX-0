/*
 * lxr0_declick.h - lxr-0: removes the step a retrigger adds to a voice that is still sounding.
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef LXR0_DECLICK_H_
#define LXR0_DECLICK_H_

#include <stdint.h>

// Called by a voice's trigger, in the render before its block. Returns 1 if the voice's last output
// sample wasn't zero. A voice whose output is exactly 0 hits as if nothing was playing.
uint8_t lxr0Declick_arm(const uint8_t voice);

// Called with a voice's output for the block, before decimation: adds the fading level, keeps the last sample.
void lxr0Declick_process(const uint8_t voice, int16_t* buf, const uint8_t size);

#endif /* LXR0_DECLICK_H_ */
