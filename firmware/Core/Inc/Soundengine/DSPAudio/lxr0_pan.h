/*
 * lxr0_pan.h - lxr-0: pan follows its value while a note sounds, on all six voices.
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef LXR0_PAN_H_
#define LXR0_PAN_H_

// once per render block, after LFOs, CV and the link: each voice's pan gains from its pan now
void lxr0Pan_update(void);

#endif /* LXR0_PAN_H_ */
