# Releases

Images handed to the user for flashing. Each is in `releases/` (not in git; rebuild from the commit).
Version: 0.<phase>.<build>. The module image shows it on the boot screen.

| Version | Image | Commit | Status |
|---------|-------|--------|--------|
| 0.1.0 | `LXRE_LXR0_BRINGUP.img` | cd3e7f8 | passed on hardware 2026-10-27 |
| 0.2.1 | `LXRE_LXR0_MODULE.img` (unversioned name) | 5ce2907 | tested 2026-10-27: jack-detect routing bug, DRUM+voice, kit 100 |
| 0.2.2 | `LXRE_LXR0_V0.2.2.img` | 3bef8fd | tested 2026-10-27: routing fixed, outs 3/4 OK, clap/cym matches renders; accent TBD |
| 0.2.3 | `LXRE_LXR0_V0.2.3.img` | 152e6aa | built: SHIFT shows accent pin levels + last velocity |
