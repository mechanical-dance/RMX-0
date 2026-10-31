# lxr-0 plan (from 2026-10-31)

Erica Synths published the module firmware (`~/code/vendor/lxr-02/lxr-eurorack-app`, GPL-3.0, v1.51).
`firmware/` is now an unchanged copy of it (vendor e752339). No more reverse engineering: their
implementation is the reference, including accents.

Goals, on top of their firmware:
1. **Samples:** LXR02Open's sample oscillator, sample memory and SD loader, mirrored as-is.
2. **MIDI:** the engine's own MIDI code (already in the tree, compiled out for the module), on the
   OP HH trigger jack (PD9 = USART3_RX) if a hardware test confirms it works.

Each change is a small, marked diff against the vendor copy, so newer Erica releases can be merged.
Images: `releases/LXRE_LXR0_V0.<n>.<build>.img`, logged in `RELEASES.md`.
