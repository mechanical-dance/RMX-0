---
title: Release notes
nav_order: 11
---

# Release notes

## 1.0.1

Update file: `LXRE_RMX0_V1.0.1.img`.

- With the global MIDI channel set to `off`, CCs are now ignored too. Before, they kept answering on the last channel set, or on channel 1 after power-up.

## 1.0

First public release. Update file: `LXRE_RMX0_V1.0.img`. Based on Erica Synths' LXR Eurorack firmware v1.51.

Includes everything below: [MIDI input](midi.md), [voice links](voice-links.md), [glide and legato](glide-legato.md), a [global reverb](reverb.md), [samples](samples.md), [new LFO shapes](lfo.md), [pan that follows sounding notes, click-free retriggers and a restart menu](other-changes.md).

Changes since 0.5.8:

- New start-up screen with the RMX-0 logo and version.
- MIDI pages show `t1`–`t7` for each voice's channel and `n1`–`n7` for its note (were `ch1`–`ch7` and `nt1`–`nt7`).
- The link mode is `lnk` (was `mod`, which also means modulation).
- Full names read *Sample Start* and *Sample End* (were *Trim*), and *Glide Time*, *Glide Type* and *Glide Legato* (were under *Link*, though glide works without a link).
- Removed the diagnostic readouts: MIDI error counts on the global page and render load on the reverb page. The reverb's load guard still works.
- Update files are named `LXRE_RMX0_V<version>.img`.

## Development versions

Not released publicly. Kits saved on any of them load in 1.0.

| Version | Changes |
|---|---|
| 0.5.8 | Pan follows sounding notes on all voices; linked drums keep their own volume |
| 0.5.7 | Glide per drum, MIDI legato, note-off releases keys for legato and poly |
| 0.5.6 | Glide types (`lcr`, `lct`, `exp`), glide always on, new time scale |
| 0.5.5 | Eleven new LFO shapes |
| 0.5.4 | Glide |
| 0.5.1–0.5.3 | Global MIDI channel follows the selected drum and plays linked groups; can be turned off |
| 0.5.0 | Voice links: unison, chord and poly |
| 0.4.14 | Fast-memory cleanup |
| 0.4.13 | Click-free retriggers; fades at sample trims |
| 0.4.12 | Coarse 0, fine 0 plays a sample at its original speed |
| 0.4.11 | Sample start and end; the waveform pot scrolls samples by name |
| 0.4.10 | MIDI input: clock and active sensing no longer break notes; recovery after lost bytes |
| 0.4.9 | Fix: LFO retrigger settings were overwritten when a kit loaded |
| 0.4.8 | Fix: samples played as DC with no pitch CV patched |
| 0.4.3–0.4.7 | Sample install fixes; reverb load guard and CPU cost |
| 0.4.2 | Restart menu |
| 0.4.0–0.4.1 | Samples |
| 0.3.2–0.3.4 | Global reverb, stereo from 0.3.3 |
| 0.3.1 | MIDI input on the OP HH jack |
