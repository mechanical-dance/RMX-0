# Releases

Images handed to the user for flashing. Each is in `releases/` (not in git; rebuild from the commit).
Version: 0.<phase>.<build>. The module image shows it on the boot screen.

| Version | Image | Commit | Status |
|---------|-------|--------|--------|
| 0.1.0 | `LXRE_LXR0_BRINGUP.img` | cd3e7f8 | passed on hardware 2026-10-27 |
| 0.2.1 | `LXRE_LXR0_MODULE.img` (unversioned name) | 5ce2907 | tested 2026-10-27: jack-detect routing bug, DRUM+voice, kit 100 |
| 0.2.2 | `LXRE_LXR0_V0.2.2.img` | 3bef8fd | tested 2026-10-27: routing fixed, outs 3/4 OK, clap/cym matches renders; accent TBD |
| 0.2.3 | `LXRE_LXR0_V0.2.3.img` | 152e6aa | tested: accent inputs are non-inverting (pin 0 unpatched, 1 with gate) |
| 0.2.4 | `LXRE_LXR0_V0.2.4.img` | 02e250c | built: accent read flipped (deviation from stock module) |

From 0.3.0 the base is Erica Synths' open-sourced module firmware (`lxr-eurorack-app`, GPL-3.0, published
2026-10-28). The 0.1/0.2 images (bring-up, desktop-engine port) are in git history before this change.

| Version | Image | Commit | Status |
|---------|-------|--------|--------|
| 0.3.0 | `LXRE_LXR0_V0.3.0_ERICA1.51.img` | 31d0c32 | Erica v1.51 (e752339) unchanged; built, not flashed |
| 0.3.1 | `LXRE_LXR0_V0.3.1.img` | f02bf00 | MIDI in on OP HH (`LXR0_MIDI`), global page sub-pages 2–3; works on hardware 2026-10-31, occasional short dropouts (cause TBD) |
| 0.3.2 | `LXRE_LXR0_V0.3.2.img` | 946fd82 | global reverb bus (`LXR0_REVERB`); built, not flashed |
| 0.3.3 | `LXRE_LXR0_V0.3.3.img` | aaef184 | 0.3.2 + stereo reverb bus (right-output taps); built, not flashed |
| 0.3.4 | `LXRE_LXR0_V0.3.4.img` | 794ac0b | 0.3.3 + reverb page fixed (sound settings reachable), second allpass loop fixed; built, not flashed |
| 0.4.0 | `LXRE_LXR0_V0.4.0.img` | 5770662 | built: samples from SD to flash as oscillator waveforms; host test passed, not yet on hardware |
| 0.4.1 | `LXRE_LXR0_V0.4.1.img` | bdbedcd | 0.4.0 + current kit autosaved before the install reset; pitch back to stock so PHX-D01 kits match; host test passed, not yet on hardware |
| 0.4.2 | `LXRE_LXR0_V0.4.2.img` | 75e82cf | 0.4.1 + restart menu (SHIFT + EDIT + PERF, fill the bar with the encoder) |
| 0.4.3 | `LXRE_LXR0_V0.4.3.img` | a371786 | 0.4.2 + sample scan fixed (FatFs lock), refusals no longer restart, reverb guard latch fixed, load readout on the reverb page |
| 0.4.4 | `LXRE_LXR0_V0.4.4.img` | 5acbf82 | 0.4.3 + reverb stage readout on the reverb 2nd screen (`amt`, `in`, `wet`), to find where the silent reverb stops |
| 0.4.5 | `LXRE_LXR0_V0.4.5.img` | db405b7 | 0.4.3 readout back (`cpu`, `rev`, `own`); reverb guard comes back below 95 %, not 90 %, which kept the reverb off on kit 103 |
| 0.4.6 | `LXRE_LXR0_V0.4.6.img` | 61a51c1 | 0.4.5 + delay lines staggered in the D-cache. The single-precision flag failed to apply here |
| 0.4.7 | `LXRE_LXR0_V0.4.7.img` | e0b730b | 0.4.6 + single-precision constants in `Reverb.c` |
| 0.4.8 | `LXRE_LXR0_V0.4.8.img` | 94aa5c4 | 0.4.7 + samples play with no CV patched: the pitch ignored `cvFreq == -1` (no CV), so the increment was 0 and every sample sat on its first frame |
| 0.4.9 | `LXRE_LXR0_V0.4.9.img` | c69c5a2 | 0.4.8 + a kit load no longer sets LFO target and retrigger bytes to 1, which made LFOs with retrigger off restart on DRUM1 |
| 0.4.10 | `LXRE_LXR0_V0.4.10.img` | 3ff39d5 | 0.4.9 + MIDI: clock and active sensing no longer break notes, `USART3` above the render interrupt, noisy bytes kept, recovery after a lost byte, error counts on the global page, trigger queue flag written last |
| 0.4.11 | `LXRE_LXR0_V0.4.11.img` | 19358da | 0.4.10 + the third pot scrolls waveforms in the name view, and each voice's envelope page has a 2nd screen trimming its sample's start and end |
| 0.4.12 | `LXRE_LXR0_V0.4.12.img` | 170cab4 | 0.4.11 + coarse 0, fine 0 plays a sample at its original speed, so a kit using a sample plays about 34 semitones lower than on 0.4.11 |
| 0.4.13 | `LXRE_LXR0_V0.4.13.img` | eec2afb | 0.4.12 + retriggering a sounding voice no longer steps the output, and sample trims fade at their start and end and crossfade at a loop point. A hit from silence and an untrimmed sample are unchanged |
| 0.4.14 | `LXRE_LXR0_V0.4.14.img` | aa44583 | 0.4.13 with the trim fade code shared between the two sample paths, so fast memory (ITCM) goes from 97 % to 90 % full. Sound unchanged: renders match 0.4.13 |
| 0.5.0 | `LXRE_LXR0_V0.5.0.img` | 6c5c7dd | 0.4.14 + voice links: drums 1-3 in unison, chord or poly, set up from the drum transient page and DRUM + two drums. Kits grow to 339 bytes; with no link the sound is unchanged |
| 0.5.1 | `LXRE_LXR0_V0.5.1.img` | 066459b | 0.5.0 + notes on the global MIDI channel follow the selected drum (they always played drum 1), and play the group when that drum is linked |
| 0.5.2 | `LXRE_LXR0_V0.5.2.img` | ddef4b9 | 0.5.1 + global MIDI channel 0 is off for notes (it stayed on channel 1 and doubled the selected drum); group LEDs only while a group drum is selected |
| 0.5.3 | `LXRE_LXR0_V0.5.3.img` | 3e2dae3 | 0.5.2 + the global MIDI channel can be turned to off (0) from the knob |
| 0.5.4 | `LXRE_LXR0_V0.5.4.img` | d5da6eb | 0.5.3 + glide (pitch mod page, screen 2): constant rate, the group in unison and chord, drum 1 when unlinked; 340-byte kits |
| 0.5.5 | `LXRE_LXR0_V0.5.5.img` | 3095106 | 0.5.4 + LFO shapes: smooth random (nse), exponential triangle (xtr), one-shots (si1 tr1 su1 sd1 sq1 xu1 xd1 xt1) and random per hit (rn1). Kits unchanged |
| 0.5.6 | `LXRE_LXR0_V0.5.6.img` | 5986a5e | 0.5.5 + glide types (lcr, lct, exp), glide always on, new glide scale (25 ms to 8 s); 341-byte kits |
