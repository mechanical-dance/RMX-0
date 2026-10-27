# Phase 2: the module's firmware, from open source

## 2a: the desktop engine played from the module's jacks
Status 2026-10-27: built, not yet flashed. Target `lxr0_module` (`firmware/module/`), output
`build/module/LXRE_LXR0_MODULE.img` (209 KB; ITCM 92 %, flash 15 %).

### What it is
The desktop engine, preset and MIDI code, unchanged, linked with module-side files:
| File | Role | Source mirrored |
|------|------|-----------------|
| `module_main.c` | boot and main loop | desktop `main()`, minus the desktop UI, USB, MIDI port |
| `module_hw.c` | clocks, I2S, SD, timers, module GPIO + trigger EXTI | desktop `MX_*_Init`; stock GPIO block `0x800bc10` |
| `module_it.c` | interrupt handlers | desktop `stm32f7xx_it.c` without USB/SPI1/front panel |
| `module_triggers.c` | trigger + accent jacks → voices | stock `vec_56`/`vec_39`, `FUN_080133dc`, `FUN_08013214` |
| `module_kit.c` | module kits 0–127 from `/proj00`, matrix tail kept | desktop `preset_loadDrumsetSD`/`preset_readDrumsetData` |
| `module_display.c` | non-blocking LCD refresh from the TIM14 tick | desktop `sendDisplayBuffer` |
| `module_panel.c` | buttons, LEDs | bring-up image (verified on hardware) |
| `module_ui.c`, `module_stubs.c` | symbols the engine expects from the desktop UI and trigger-out code | — |

Left out on purpose: `TriggerOut.c` (desktop trigger outs drive pins that are LEDs here, and it claims
EXTI9_5); the desktop menu, LED and button code (shift registers); USB device init; USART3.
The kit's per-LFO and velocity targets are dropped in `frontPanel_sendData`, as the module ignores them
(confirmed on hardware 2026-10-26). Globals (`glo.cfg`) are read but not applied: the module's
31-byte layout isn't decoded yet. Nothing writes to the SD card.

### Flash and test (the user does this)
Same as the bring-up: only `LXRE_LXR0_MODULE.img` on the card, plus the card's usual `PROJ00` and
`STATE.IDX`. Hold the encoder at power-on. Recovery: the stock image the same way.

| Check | Pass |
|------|------|
| Boot | `lxr-0 module / Phase 2a`, then the kit screen |
| Line 1 | `120 <name>`: the kit from `STATE.IDX` (120 on the 2026-10-25 card); ` m` if it has a mod matrix |
| Line 2 | `cpu NN% pk NNNN`: engine share and worst single engine call in µs. The block budget is 1451 µs (64 samples at 44.1 kHz) |
| Gate into a trigger jack | That voice plays; its voice button LED blinks |
| Same with the accent jack high | Louder hit (velocity 127 vs 63) |
| DRUM held + a voice button | Plays that voice |
| Turn the encoder | Browse kits; the kit loads when the encoder stops (a short gap is expected) |
| All 4 outputs | Each has audio, per the kit's routing |
| A/B vs stock | Same kit, same triggers: should sound the same, except the mod matrix (not applied yet, 2c) |

Things to note for me: any voice that's silent or plays on the wrong jack, accent not working,
clicks or dropouts while playing (not while loading), the `cpu`/`pk` numbers while triggering all 7.

### First hardware test (2026-10-27, the user)
- Boots; triggers play; encoder changes kits. CPU up to 77 %, `pk` up to ~900 µs of 1451 (the penalty kit,
  ~74 %). Load follows FX sends and filter resonance more than trigger rate.
- **Fixed in build 2:**
  - Accents ducked voices, out 4 seemed dead, and the clap/cymbal was faint. Cause: the desktop mixer
    reads output-jack switch pins (PD6, PD7, PB4, PB6) and reroutes voices to "plugged" jacks. On the
    module those pins are two accent inputs, an LED and the DRUM button. The stock mixer (`itcm_20c4`)
    doesn't read them; `mixer.c` now skips them under `LXR_MODULE` (module build only, via `lxr0_dsp`).
    PHX-D01 renders every card kit's cymbal at full level, so the engine itself was fine.
  - DRUM + voice button didn't play: the scan returned the first pressed button (DRUM).
  - Kit 100 showed a garbage name: it's `100-TMP.SND`, a 302-byte stock scratch file. Only 255/309-byte
    files load now.
  - Kit browsing: the kit list is indexed at boot, the encoder browses (`> NNN NAME`) and the kit loads
    400 ms after the encoder stops; the DMA buffers are silenced during the load instead of looping.

### Next
- 2b: pots, CVs (ADC with the module ranks, its own MSP without PA5), and the module's menu and edit flow
  from the manual; kit save; `glo.cfg`.
- 2c: the mod matrix from the kit's 54-byte tail (`moduleKit_matrix`), with the target table at `0x28380`;
  `v/o` (`FUN_080115c0`).
