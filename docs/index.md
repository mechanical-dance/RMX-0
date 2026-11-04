---
title: Home
nav_order: 1
---

# RMX-0

RMX-0 is custom firmware for the Erica Synths LXR Eurorack drum module. It is built on Erica Synths' open-source module firmware (v1.51) and keeps everything the stock firmware does.

RMX-0 is unofficial. It is not made, approved or supported by Erica Synths.

## What it adds

| Feature | Summary |
|---|---|
| [MIDI input](midi.md) | The OP HH jack becomes a MIDI input, with a channel and note per voice |
| [Voice links](voice-links.md) | Drums 1–3 play as one instrument in unison, chord or poly mode |
| [Glide and legato](glide-legato.md) | Per-drum portamento with three curves, and MIDI legato |
| [Global reverb](reverb.md) | A stereo reverb with per-voice sends, alongside the FX bus |
| [Samples](samples.md) | Your own WAV files as oscillator waveforms, with start and end trims |
| [LFO shapes](lfo.md) | Eleven new shapes, including smooth random and one-shot envelopes |
| [Other changes](other-changes.md) | Pan that moves sounding notes, click-free retriggers, a restart menu |

## Kits

- Kits from the stock firmware load unchanged. The added settings start off or at 0, so they sound the same.
- RMX-0 stores its settings after the stock part of the kit file. The stock firmware reads only its own part, so RMX-0 kits still load there, without the additions.
- Kits that use samples, links or glide need RMX-0 to sound as saved.

## Getting started

1. [Install RMX-0](install.md) from an SD card.
2. Read the page for the feature you want to use.
3. To build it yourself, see [Building from source](building.md).

[Credits and licence](credits.md) · [Release notes](release-notes.md)
