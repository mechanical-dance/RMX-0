/*
 * lxr0_voiceLink.h - lxr-0: drums 1-3 linked as one instrument (unison, chord or poly).
 *
 * Copyright (C) 2026 lxr-0, on top of the LXR firmware (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef LXR0_VOICELINK_H_
#define LXR0_VOICELINK_H_

#include <stdint.h>

// once per render block: picks up changed link settings, wherever they came from
void lxr0Link_poll(void);
// once per render block, after triggers and CV: glide each drum's, or group's, pitch
void lxr0Link_glide(void);
// a trigger for a voice. Returns 1 if the link handled it, 0 if it plays as usual.
uint8_t lxr0Link_noteOn(uint8_t voice, uint8_t note, uint8_t vel);
// 0.5.7: a MIDI note for a voice, after note matching (key is the note as it came in). Keeps the key held;
// returns 1 if the link played it (legato, or a linked drum), 0 if it plays as usual.
uint8_t lxr0Link_midiNoteOn(uint8_t voice, uint8_t chan, uint8_t key, uint8_t note, uint8_t vel);
// a MIDI note-off (or note-on at velocity 0): lets go of the key wherever it is held
void lxr0Link_midiNoteOff(uint8_t chan, uint8_t key);
// pitch CV for drum 1-3. Returns 1 if the link handled it.
uint8_t lxr0Link_cv(uint8_t voice, float freq);
// the drums linked now (bit 0-2), 0 if none
uint8_t lxr0Link_voices(void);
// the lowest linked drum, which takes the trigger, accent and pitch CV
uint8_t lxr0Link_lead(void);
// the voice that plays a note meant for this one: the lead if the voice is linked, else the voice itself
uint8_t lxr0Link_route(uint8_t voice);
// a linked drum edited on its own (double-tap): the menu's group edits stop while it's set
uint8_t lxr0Link_solo(void);
void lxr0Link_setSolo(uint8_t on);

#endif /* LXR0_VOICELINK_H_ */
