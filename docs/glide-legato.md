---
title: Glide and legato
nav_order: 5
---

# Glide and legato

Glide and legato are on the second screen of the pitch mod page of drums 1–3. Each drum has its own settings.

## Glide

| Setting | Values | Effect |
|---|---|---|
| `gld` Glide time | 0–127 | Time T from 25 ms (1) to 8 s (127), about 0.45 s at 64. 0 is off |
| `typ` Glide type | `lcr`, `lct`, `exp` | How the pitch moves (below) |

| Type | Behaviour |
|---|---|
| `lcr` Linear, constant rate | T per octave, even in pitch. Bigger jumps take longer: an octave takes 12 times as long as a semitone |
| `lct` Linear, constant time | Every slide takes T, whatever the interval |
| `exp` Exponential | An RC lag: fast at first, then easing into the note, within 1 % of it after T |

- **Always on:** like an analog portamento, it glides on every pitch change, through silence and repeated hits. A new note in the middle of a slide continues from wherever the pitch is.
- **What slides:** in unison and chord, the whole group, with detune and chord intervals kept, using the lead's settings. A drum outside a link slides on its own. Poly doesn't slide.
- **What it follows:** note pitch, pitch CV and tuning edits. A jump in pitch CV slides like a new note. Slow CV movement, such as vibrato, is smoothed more as glide goes up.
- The pitch envelope, transient and FM still play on every note.

## Legato

`lgt` (`off`, `on`) works with MIDI notes only.

- **On:** a note played while another key is held, and while the drum still sounds, changes only the pitch. The envelopes, transient, LFO and filter carry on, and glide slides to the new pitch.
- **Releasing the newest key** while others are held returns to the newest one still held.
- **A new note after all keys are up,** or after the sound has ended, plays as a new hit.
- **Groups:** unison and chord play legato with the lead's setting. Poly doesn't play legato.
- Triggers and CV always play a new hit.
