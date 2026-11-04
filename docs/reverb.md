---
title: Global reverb
nav_order: 6
---

# Global reverb

A stereo reverb on its own bus, separate from the FX bus, so both run at once. Its settings are saved with the kit. Kits from the stock firmware load with every send at 0.

| Where | Setting |
|---|---|
| Each voice's MIX page, 2nd screen | `rev`: send, after the voice's volume. The two hi-hats share one send |
| Perform page, 4th pot | `rev`: return level |
| FX button, pressed until the reverb page | `pre` pre-delay, `dcy` decay, `ton` tone, `ref` reflection |
| FX button, next press | `out`: where the reverb goes (St1 by default; mono outputs get the sum) |

**Load guard:** if the module's render load passes 95 %, the reverb is bypassed so the kit doesn't drop out. It returns once there is room again. A reverb that disappears on a heavy kit is the guard at work.
