---
title: Other changes
nav_order: 9
---

# Other changes

## Pan follows sounding notes

On all six voices, pan moves a note that is already sounding: the pan knob, an LFO or CV through the mod matrix, and a voice link's width. On the stock firmware, pan is read only when a note starts. Per-hit random pan (such as `rn1`) still works, since each hit starts wherever the LFO is. Fast pan LFOs step at the control rate (about 690 Hz), which can add a faint buzz at audio rates.

## Click-free retriggers

Retriggering a voice that is still sounding no longer steps or clicks. The old note's level fades out over 3 ms under the new one, and the new note starts as it would from silence. Hits from silence are unchanged, and no delay is added.

## Restart menu

Restarts the module without powering down the rack, for example after swapping the SD card. The card can be pulled while the module plays.

1. Hold **SHIFT** and press **EDIT** and **PERF**, in either order.
2. Turn the encoder right to fill the bar; left backs off. A full bar restarts the module.
3. Any button leaves without restarting. Audio keeps playing while the menu is open.

The current kit is saved first if a card is in. Hold the encoder as the bar fills to enter the firmware updater.

