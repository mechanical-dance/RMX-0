---
title: Samples
nav_order: 7
---

# Samples

Your own WAV files play as oscillator waveforms, after the built-in ones. They are installed from the SD card into the module's flash, where they stay through power cycles.

A firmware update erases them. Reinstall after every update.

## Installing

1. Put WAV files in `/samples` (one-shots) and `/loops` (always looped) on the card. Up to 120 files in total: 16- or 24-bit, mono or stereo, any sample rate.
2. Press **LOAD/SAVE** until `Load:Samples`, then press the encoder.
3. The current kit is saved, the flash is rewritten, and the module restarts.

**Space:** 1.5 MB. Each sample is stored as mono 16-bit at its own rate, so a second costs about 86 kB at 44.1 kHz and 43 kB at 22.05 kHz. Files that don't fit or can't be read are skipped and counted.

## Playing

- Turn a voice's waveform past `Pwm` to `s0`, `s1` and so on. Press the encoder to see the sample's name (8.3 names; a long name shows as `NAME~1`). In that view, the pot under the waveform scrolls through the samples.
- Coarse 0, fine 0 plays a sample at its original speed. Each coarse step is a semitone.
- The amp envelope still applies, so long samples need a long decay.
- Kits store the waveform number, so changing the files on the card can change which sample a kit plays.

## Start and end

The envelope page's 2nd screen has `sta` (sample start) and `end` (sample end), per voice and saved in the kit.

- Each cuts up to the whole length from its side. A loop repeats only the part between them.
- Trimmed edges fade: 1 ms in, 3 ms out, and a 5 ms crossfade at a loop point. Untrimmed samples play exactly as recorded.
- Trims apply to the main oscillator only, and the two hi-hats share one trim.
