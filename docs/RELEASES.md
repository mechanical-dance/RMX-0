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
| 0.4.1 | `LXRE_LXR0_V0.4.1.img` | COMMIT | 0.4.0 + current kit autosaved before the install reset; pitch back to stock so PHX-D01 kits match; host test passed, not yet on hardware |
